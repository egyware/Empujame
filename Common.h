#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#if defined( __WIN32__ ) || defined( _WIN32 )
#define WINDOWS
#elif defined(__APPLE__)
#define MAC
#else
#define LINUX
#endif

#ifdef WINDOWS
#include <windows.h>
#else
#define closesocket(x) close(x)
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

//#include <Box2D/Box2D.h>

#define MAX_CLIENTS 4
#define SCALE 0.01f
#define INV_SCALE 100.0f
#define TRY_AGAIN_LOGIN 5


struct Vector {
    float x,y; //Direccion del vector!!
};
/// \brief Razones porque se fue un cliente
enum Razon {
    Irse,
    Timeout
};

enum pdu_tipo {
    pdu_ping, //pdu para revisar si el cliente/servidor esta vivo :P
    pdu_login, //envio mi nick para logearme
    pdu_server, //pdu general de datos del servidor
    pdu_client, //pdu general de datos del cliente
    pdu_info, //Antes de entrar al loby
    pdu_disconnect, //Desconectarme
    pdu_search, //la envia el cliente y recibe una pdu info
    pdu_winner, //esta pdu se le envia al ganador :)

    pdu_count //pdu invalida usada solo para contar xD
};

/// \brief Estructura que sirve para revisar si el servidor/cliente sigue vivo
struct pdu_data_ping {
    pdu_tipo tipo; //!< Debe ser pdu_ping
    unsigned int id; //!< Id del jugador
    bool serverRequest; //!< El servidor pregunta
};

/// \brief para conectarse al juego
struct pdu_data_login {
    pdu_tipo tipo; //!< Tipo de dato debe ser pdu_login
    char nick[128]; //!< Nick del jugador
    unsigned int id; //!< Id del jugador, es 0 cuando intenta logearse (distinto de 0, para notificar otros jugadores)
};

/// \brief esta es la pdu que recibe el cliente
struct pdu_data_server {
    pdu_tipo tipo;
    unsigned int turno;//!< turno, sirve de stampa de tiempo
    unsigned int id; //tu ID
    unsigned int choque; //choque
    unsigned int caida; //me cai yo?
    Vector posiciones[MAX_CLIENTS];
    Vector velocidad[MAX_CLIENTS];
};

/// \brief esta es la pdu que recibe el servidor
struct pdu_data_client {
    pdu_tipo tipo;
    unsigned int id; //!< 0 para login
    unsigned int turno; //!< turno
    Vector direccion;  //!< Direccion
};

/// \brief envia la informacion del cuando está en preparando el juego
struct pdu_data_info {
    pdu_tipo tipo;
    unsigned int max_players; //!< Maximo jugadores para la partida
    unsigned int players; //!< Cantidad de jugadores en el juego
    char name[64]; //!< Nombre de la partida
    //-- más datos de información agregar aqui xD como por ejemplo la version del juego
};

/// \brief el cliente envia esta pdu cuando quiere irse. El server notifica a los demás jugadores que un jugador se fue...
struct pdu_data_disconnect {
    pdu_tipo tipo; //!< Tipo de la pdu, debe ser pdu_disconect
    unsigned int id; //!< Id de quien se fue o quiere irse.
    Razon razon;
};

struct pdu_data_search {
    pdu_tipo tipo; //!< Tipo de la pdu, debe ser pdu_search
    //que dato puedo agregar aqui???
};

struct pdu_data_winner{
    pdu_tipo tipo; //!< Tipo de la pdu, debe ser pdu_winner
    //que dato puedo agregar aqui???
};


/// \brief pdu general de datos
union pdu_data {
    pdu_tipo tipo; //!< Tipo de pdu
    pdu_data_ping ping; //!< Para ping
    pdu_data_login login; //!< Para login
    pdu_data_server server; //!< Para envio del servidor datos general
    pdu_data_client client; //!< Para envio de datos generales del cliente
    pdu_data_info info; //!< Informacion de la partida
    pdu_data_disconnect disconnect; //!< Desconeccion
    pdu_data_search serch; //!< buscar partida
};





class b2Body; //fowardclass
///  Cliente conetado
struct Client {
    unsigned int id; //!< id del cliente
    unsigned int last_data; //!< Tiempo del ultimo paquete que llego
    char nick[128];
    in_addr ip; //!< ip del cliente
    unsigned short port; //!< puerto del cliente
    Vector direccion; //!< Direccion del movimiento
    bool caida;
    b2Body *body;
    Client() {
        id = 0;
        caida = false;
        body = NULL;
    }
};

bool hacerNoBloqueante(int sock);
void Wait(int);
unsigned int getTicks();

#endif // COMMON_H_INCLUDED
