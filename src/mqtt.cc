#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <map>

#include "mqtt.h"
#include "log.h"
#include "json.hpp"

#define DIFF_TIMESTAMP 30 
 
MQTT::MQTT() : _mq(NULL)
{
    memset(this->_clientid, 0, sizeof(this->_clientid));
    sprintf(this->_clientid, "GPS_%08d", getpid());

    mosquitto_lib_init();
}

MQTT::~MQTT()
{
    if( this->_mq ) {
        mosquitto_destroy(this->_mq);
    }

    mosquitto_lib_cleanup();
}

bool MQTT::connect()
{
    int rc=0;

    this->_mq = mosquitto_new(this->_clientid, true, this);

    if( ! this->_mq  ) {
       logger("Error realizando new mq");
       return false;
    }

    mosquitto_connect_callback_set(this->_mq, MQTT::proc_connect_callback);
    mosquitto_message_callback_set(this->_mq, MQTT::proc_message_callback);

    memset(_mqhost, 0, sizeof(_mqhost));
    memset(_mquser, 0, sizeof(_mquser));
    memset(_mqpasswd, 0, sizeof(_mqpasswd));

    if( getenv("MQ_HOST") ) {
       strcpy(_mqhost, getenv("MQ_HOST"));
    } else {
       strcpy(_mqhost, "localhost");
    }

    if( getenv("MQ_PORT") ) {
        _mqport=atoi(getenv("MQ_PORT"));
    } else {
        _mqport=1883;
    }

    if( getenv("MQ_USER") ) {
       strcpy(_mquser, getenv("MQ_USER"));
    }

    if( getenv("MQ_PASSWD") ) {
       strcpy(_mqpasswd, getenv("MQ_PASSWD"));
    }

    logger("Conectandose al host: %s puerto: %d", _mqhost, _mqport);

    if( strlen(_mquser) > 0 ) {
        logger("Usuario: %s ", _mquser);
        mosquitto_username_pw_set(this->_mq, _mquser, _mqpasswd);
    }

    rc = mosquitto_connect(this->_mq, _mqhost, _mqport, 60);

    if( rc != MOSQ_ERR_SUCCESS ) {
        logger("Error conectando: %s", this->error(rc));
	return false;
    }

    return true;
}

void MQTT::cargarColas()
{
    if( ! this->_db ) {
        logger("Error. Sin base de datos");
	return;
    }

    this->_listaColas.clear();

    if( ! this->_db->cargarColas(this->_listaColas) == -1 ) {
        logger("Error cargando colas.");
    }
}

bool MQTT::subscribe(const char* name)
{
    int rc=0;
    char queue[400];

    memset(queue, 0, sizeof(queue));

    sprintf(queue, "%s/+", name);

    rc=mosquitto_subscribe(this->_mq, NULL, queue, 0);

    if( rc != MOSQ_ERR_SUCCESS ) {
        logger("Error subscribiendo: %s", this->error(rc));
	return false;
    }

    return true;
}

bool MQTT::unsubscribe(const char* name)
{
    int rc=0;
    char queue[400];

    memset(queue, 0, sizeof(queue));

    sprintf(queue, "%s/+", name);

    rc=mosquitto_unsubscribe(this->_mq, NULL, queue);

    if( rc != MOSQ_ERR_SUCCESS ) {
        logger("Error unsubscribiendo: %s", this->error(rc));
	return false;
    }

    return true;
}

bool MQTT::loop()
{
    int rc;

    rc = mosquitto_loop(this->_mq, -1, 1);

    if(rc) {
        logger("Error en conexion, intentando reconectar en 10 segundos.");
        sleep(10);
//        mosquitto_reconnect(this->_mq);
          this->connect();
    }

    return true;
}

bool MQTT::unsubscribeAll()
{
  for( std::map<std::string, std::string>::iterator it = this->_listaColas.begin();
                                                     it != this->_listaColas.end(); it++ ) {

    if( ! this->unsubscribe(it->first.c_str() ) ) {
        logger("Error unsubscribiendo en la cola  %s para el IMEI: %s", it->first.c_str(), it->second.c_str());
    } else {
        logger("Desubscribiendo IMEI: [%s] Cola: [%s]\n", it->second.c_str(), it->first.c_str());
    }
  }

  return true;

}

bool MQTT::subscribeAll()
{
    int rc=mosquitto_subscribe(this->_mq, NULL, "#", 0);

    if( rc != MOSQ_ERR_SUCCESS ) {
        logger("Error subscribiendo: %s", this->error(rc));
	return false;
    }

/*  for( std::map<std::string, std::string>::iterator it = this->_listaColas.begin();
                                                     it != this->_listaColas.end(); it++ ) {

    if( ! this->subscribe(it->first.c_str() ) ) {
        logger("Error subscribiendo en la cola  %s para el IMEI: %s", it->first.c_str(), it->second.c_str());
    } else {
        logger("Subscripto IMEI: [%s] Cola: [%s]\n", it->second.c_str(), it->first.c_str());
    }
  }*/

  return true;
}

void MQTT::connect_callback(struct mosquitto *mosq, int result)
{
    //printf("connect callback, rc=%d\n", result);
    if( result == 0 ) {
        logger("Conectado exitosamente");
    } else {
        logger("Error conectando. Codigo: %d [%s]", result, error(result));
    }
}

void MQTT::message_callback(struct mosquitto *mosq, const struct mosquitto_message *message)
{
    int pos=0;
    bool match = 0;
    char device[300], param[300], *buffer;

    memset(device, 0, sizeof(device));
    memset(param, 0, sizeof(param));

    if( message->topic[0] == '/' ) {
        
        pos=strcspn( &message->topic[1], "/");

        strncpy(device, &message->topic[1], pos);
        strcpy(param, &message->topic[pos+2]);
    } else {
        pos=strcspn( message->topic, "/");

        strncpy(device, message->topic, pos);
        strcpy(param, &message->topic[pos+1]);
    }

//    printf("DEVICE: %s PARAM: %s\n", device, param);

    buffer=new char[message->payloadlen+1];

    memset(buffer, 0, message->payloadlen+1);

    memcpy(buffer, message->payload, message->payloadlen);

    this->process_message(device, param, buffer);

    delete []buffer;
}

void MQTT::process_message(char* device, char* param, char* value ) 
{
    using json = nlohmann::json;
    double angle=0.0, speed=0.0, lat=0.0, lng=0.0;

    int tActual = this->_db->getTimestamp(), tInicio, tFinal, tFecha;
    char params[2000];

    tInicio = tActual - DIFF_TIMESTAMP;
    tFinal = tActual + DIFF_TIMESTAMP;
    memset(params, 0, sizeof(params));

    if( ! this->_db->getImeiValues( device, &lat, &lng, &angle, &speed) ) {
        logger("Datos de posicion no encontrado para %s\n", device);
    }

    
    if( this->_db->obtenerParams( device,params) ) {
        json j;

        if( strlen(params) > 0 ) {
            j = json::parse(params);
        } else {
            j = json::parse("{}");
        }

        j[param] = value;

        this->_db->actualizarParams( device, this->_mqhost, this->_mqport, j.dump().c_str(), lat, lng, angle, speed);
    } else {
        json j = json::parse("{}");

        j[param] = value;

        this->_db->insertGsObjects( this->_mqhost, this->_mqport, device, j.dump().c_str(), lat, lng, angle, speed);
    }

    tFecha = 0;

    json j = json::parse("{}");

    j[param] = value;

    this->_db->insertGsObjectHistorico( device, j.dump().c_str(), lat, lng, angle, speed);

}

void MQTT::proc_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    MQTT* pmqtt = (MQTT*)obj;

    pmqtt->connect_callback(mosq, result);

}

void MQTT::proc_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    MQTT* pmqtt = (MQTT*)obj;

    pmqtt->message_callback(mosq, message);
}

char* MQTT::error(int rc)
{
    static char error[1000];

    memset(error, 0, sizeof(error));

    switch(rc) {

       case MOSQ_ERR_SUCCESS:
           strcpy(error, "on success.");
           break;
       case MOSQ_ERR_INVAL:
           strcpy(error, "if the input parameters were invalid.");
           break;
       case MOSQ_ERR_NOMEM:
           strcpy(error, "if an out of memory condition occurred.");
           break;
       case MOSQ_ERR_NO_CONN:
           strcpy(error, "if the client isn’t connected to a broker.");
           break;
       case MOSQ_ERR_MALFORMED_UTF8:
           strcpy(error, "if the topic is not valid UTF-8");
           break;
    }

    return error;
}

