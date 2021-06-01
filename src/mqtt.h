#ifndef __MQTT_INC__
#define __MQTT_INC__

#include <mosquitto.h>
#include "db.h"

class MQTT {
    public:
        MQTT();
        ~MQTT();
    public:
	bool connect();
	bool subscribe(const char* queue);
	bool unsubscribe(const char* queue);
	bool subscribeAll();
	bool unsubscribeAll();
	bool loop();
	void setDB(DB* db) { this->_db = db; }
	void cargarColas();

    private:
	char _clientid[200];
        char _mqhost[200];
        char _mquser[200];
	char _mqpasswd[200];
        int _mqport;
        struct mosquitto *_mq;
        DB* _db;
        std::map<std::string, std::string> _listaColas;

        char* error(int rc);
	void connect_callback(struct mosquitto *mosq, int result);
        void message_callback(struct mosquitto *mosq, const struct mosquitto_message *message);
        void process_message(char* device, char* param, char* value );

	static void proc_connect_callback(struct mosquitto *mosq, void *obj, int result);
        static void proc_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
};

#endif  /*  __MQTT_INC__  */
