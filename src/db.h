#ifndef __DB_INC__
#define __DB_INC__

#include <mysql/mysql.h>
#include <map>

class DB {
    public :
        DB();
        ~DB();

    public:
       bool connect();
       int cargarColas(std::map<std::string, std::string>& listaColas);
       int getTimestamp();
       bool obtenerParams( const char* imei, char* param);
       bool actualizarParams(const char* imei, char* server, int port, const char* param, double lat, double lng, double angle, double speed );
       bool actualizarParamsHistoricos(const char* imei, int treg, const char* param );
       bool obtenerParamsHistorico( const char* imei, int tInicio, int tFinal, int *id, char* param);
       bool insertGsObjects(char* server, int port, char* imei, const char* params, double lat, double lng, double angle, double speed);
       bool insertGsObjectHistorico(char* imei, const char* params, double lat, double lng, double angle, double speed);
       bool getImeiValues(char* device, double* lat, double* lon, double* pAngle, double* pSpeed);

    protected :
       MYSQL _mysql;
};


#endif /*  __DB_INC__ */
