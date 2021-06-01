#include <cstdio>
#include <map>

#include "log.h"
#include "db.h"
#include "mqtt.h"

#define SEG_QUEUE_UPD  300

int main(int argc, char** argv)
{
  int contador=SEG_QUEUE_UPD;
  MQTT mqtt;
  DB db;
  std::map<std::string, std::string> listaColas;

  if( ! db.connect() ) {
    logger("Error conectando a la base");
    exit(-1);
  }

  if( ! mqtt.connect() ) {
    logger("Error conectando al MQTT");
    exit(-1);
  }

  mqtt.setDB(&db);

  mqtt.subscribeAll();

  while(1) {

/*    if( contador >= SEG_QUEUE_UPD ) {

        mqtt.unsubscribeAll();

        mqtt.cargarColas();

        mqtt.subscribeAll();
        contador=0;
    }*/

    mqtt.loop();
    contador++;
  }

  return 0;  
}
