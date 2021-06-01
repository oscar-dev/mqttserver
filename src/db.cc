#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctype.h>

#include "db.h"
#include "log.h"

#include "json.hpp"

DB::DB()
{
    mysql_init(&this->_mysql);
}

DB::~DB()
{
    mysql_close(&this->_mysql);
}

bool DB::connect() 
{
    bool ret=false;
    const char *dbname=getenv("DB_NAME"),
                     *dbserver=getenv("DB_HOST"),
                     *dbuser=getenv("DB_USER"),
                     *dbpasswd=getenv("DB_PASSWD");

    logger("Conectando a la base %s host: %s usuario: %s ", dbname, dbserver, dbuser);

    if( ! mysql_real_connect( &this->_mysql, dbserver, dbuser, dbpasswd, dbname, 0, NULL, 0) ) {

        logger("Error estableciendo la conexion a base de datos. Error: %s", mysql_error(&this->_mysql));

        ret = false;
    } else {
        ret = true;
    }

    return ret;
}

int DB::getTimestamp()
{
    char sql[1000];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "SELECT UNIX_TIMESTAMP()");

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error consultando fecha y hora. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    result = mysql_store_result(&this->_mysql);

    if( ! result ) {
        logger("Error en store result. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    row = mysql_fetch_row(result);

    if ( row ) {
        mysql_free_result(result);
        return atoi(row[0]);
    } else {
        mysql_free_result(result);
        return -1;
    }
}

int DB::cargarColas(std::map<std::string, std::string>& listaColas) 
{
    int contador=0;
    char sql[1000];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "SELECT id, imei, queue FROM gs_rel_devices");

    logger("Comenzando a cargar colas");

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error consultando tablas de colas. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    result = mysql_store_result(&this->_mysql);

    if( ! result ) {
        logger("Error en store result. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    while ( row=mysql_fetch_row(result) ) {

        listaColas[row[2]]=row[1];

	contador++;
    }

    mysql_free_result(result);

    return contador;
}

bool DB::obtenerParams(const char* imei, char* param) 
{
    bool ret=false;
    char sql[1000];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "SELECT params FROM gs_objects WHERE IMEI='%s'", imei);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error consultando imei en gs_objects. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    result = mysql_store_result(&this->_mysql);

    if( ! result ) {
        logger("Error en store result. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    if ( row=mysql_fetch_row(result) ) {
        strcpy( param, row[0]);
        ret=true;
    }

    mysql_free_result(result);

    return ret;
}

bool DB::actualizarParams(const char* imei, const char* param )
{
    bool ret=false;
    char sql[3100], buffer[3000];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(buffer, 0, sizeof(buffer));
    
    mysql_real_escape_string(&this->_mysql, buffer, param, strlen(param));

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "UPDATE gs_objects SET params='%s', dt_server=NOW(), dt_tracker=NOW() WHERE IMEI='%s'", buffer, imei);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error actualizando params en gs_objects. Error: %s", mysql_error(&this->_mysql));
	return false;
    }

    return true;
}

bool DB::obtenerParamsHistorico(const char* imei, int tInicio, int tFinal, int * treg, char* param) 
{
    bool ret=false;
    char sql[3100], buffer[3000], imeicase[500];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(buffer, 0, sizeof(buffer));
    memset(imeicase, 0, sizeof(imeicase));

    for( int i=0; i < strlen(imei); i++ ) {
        imeicase[i] = toupper(imei[i]);
    }
    
    mysql_real_escape_string(&this->_mysql, buffer, param, strlen(param));

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "SELECT UNIX_TIMESTAMP(dt_server), params FROM gs_object_data_%s WHERE dt_server BETWEEN FROM_UNIXTIME(%d) AND FROM_UNIXTIME(%d) ",
                                                                                                                           imeicase, tInicio, tFinal);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error actualizando params en gs_object_data_%s. Error: %s", imeicase, mysql_error(&this->_mysql));
	return false;
    }

    result = mysql_store_result(&this->_mysql);

    if( ! result ) {
        logger("Error en store result. Error: %s", mysql_error(&this->_mysql));
	return -1;
    }

    if ( row=mysql_fetch_row(result) ) {
        strcpy( param, row[1]);
	*treg = atoi(row[0]);
        ret=true;
    }

    mysql_free_result(result);

    return ret;
}

bool DB::actualizarParamsHistoricos(const char* imei, int treg, const char* param )
{
    bool ret=false;
    char sql[3100], buffer[3000], imeicase[500];
    MYSQL_RES *result;
    MYSQL_ROW row;

    memset(buffer, 0, sizeof(buffer));
    memset(imeicase, 0, sizeof(imeicase));

    for( int i=0; i < strlen(imei); i++ ) {
        imeicase[i] = toupper(imei[i]);
    }

    mysql_real_escape_string(&this->_mysql, buffer, param, strlen(param));

    memset(sql, 0, sizeof(sql));
    sprintf( sql, "UPDATE gs_object_data_%s SET params='%s' WHERE dt_server=FROM_UNIXTIME('%d')", imeicase, buffer, treg);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error actualizando params en gs_object_data_%s. Error: %s", imeicase, mysql_error(&this->_mysql));
	return false;
    }

    return true;
}

bool DB::insertGsObjects(char* server, int port, char* imei, const char* params)
{
    char sql[5000], buffer[3000];

    memset(sql, 0, sizeof(sql));
    memset(buffer, 0, sizeof(buffer));

    mysql_real_escape_string(&this->_mysql, buffer, params, strlen(params));


    mysql_query( &this->_mysql, "SET sql_mode = 'allow_invalid_dates'");

    sprintf( sql, "INSERT INTO gs_objects(imei, net_protocol, ip, port, dt_server, dt_tracker,"
                        "lat, lng, altitude, angle, speed, loc_valid, params, protocol, active, object_expire, "
                        "object_expire_dt, manager_id, dt_last_stop, dt_last_idle, dt_last_move, name,"
                        "icon, map_arrows, map_icon, tail_color, tail_points, device,sim_number,model,vin,"
                        "plate_number,odometer_type,engine_hours_type,fcr,time_adj, accuracy, dt_chat, odometer, engine_hours) "
                        "VALUES ( '%s', 'mq', '%s', '%d', NOW(), NOW(), 0, 0, 0, 0, 0, '1', '%s', 'mqtt', 'true', 'false', "
			"'0000-00-00', 0, NOW(), '0000-00-00', NOW(), "
			"'%s', '../img/user.svg', '', 'arrow', '#00FF44', '7', '', '', 'null', '', '', 'gps', 'off', '', '', '', "
			"'0000-00-00', '0', '0')", imei, server, port, buffer, imei);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error insertando params en gs_objects. IMEI: %s. Error: %s", imei, mysql_error(&this->_mysql));
	return false;
    }

    return true;
}

bool DB::insertGsObjectHistorico(char* imei, const char* params)
{
    char sql[5000], buffer[3000], imeicase[500];

    memset(sql, 0, sizeof(sql));
    memset(buffer, 0, sizeof(buffer));
    memset(imeicase, 0, sizeof(imeicase));

    for( int i=0; i < strlen(imei); i++ ) {
        imeicase[i] = toupper(imei[i]);
    }

    mysql_real_escape_string(&this->_mysql, buffer, params, strlen(params));

    sprintf( sql, "INSERT INTO gs_object_data_%s(dt_server, dt_tracker, lat, lng, "
		    "altitude, angle, speed, params) VALUES( NOW(), NOW(), 0, 0, 0, 0, 0, '%s')", imeicase, params);

    if( mysql_query( &this->_mysql, sql) ) {
        logger("Error insertando params en gs_object_data_%s. Error: %s", imeicase, mysql_error(&this->_mysql));
	return false;
    }

    return true;
}
