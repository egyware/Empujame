#include <cstdio>
#include <cstdlib>
#include <cstring>

//#include <SDL/SDL.h>

#if defined( __WIN32__ ) || defined( _WIN32 )
#define WINDOWS
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

#define MAX_CLIENTS 4

enum EAction_Type {
    EAT_Move,//!< Moverse
    EAT_Shot,//!< Disparar
    EAT_Access,//!< Acceder a terminal
    EAT_Die, //!< Tu mueres o alguien
    EAT_Message, //!< Mensaje
    EAT_Change_Chanel, //!< Cambiar canal;
    EAT_Reject_User, //!< Rechazar usuario
    EAT_Terminal_Message, //!< Mensaje del terminal

    EAT_Action_Count //!< Contador de mensajes, no se usa
};
struct Action_Vector {
    int x,y; //!< Direccion (para Move y Access)
};
struct Action_Client {
    unsigned int id; //!< ID de quien murio
};
struct Action_Message {
    unsigned char channel;
    char data[512];
};
struct Action_Terminal_Message {
    /// Por hacer :'(
};
struct Action {
    union {
        Action_Vector vector;
        Action_Client client;
        Action_Message message;
        Action_Terminal_Message terminal;
    };
    Action() {}
};

struct pdu_server {
    EAction_Type type; //!< tipo mensaje
    struct Action action; //!< accion que le ocurrio al jugador
    unsigned int turn;//!< turno, sirve de stampa de tiempo
    unsigned char vision[9]; //!< el servidor siempre entregara lo que vez...
    pdu_server(int t,EAction_Type at):type(at) {
        turn = t;//turno
        memset(vision,-1,sizeof(vision));
    }
};
struct pdu_login{
    EAction_Type type;
    Action_Client client;
};
struct pdu_client {
    unsigned int id; //!< 0 para login
    Action action; //!< Accion a realizar, si es 0 es login
};

///  Cliente conetado
struct Client{
    unsigned int id; //!< id del cliente
    in_addr ip; //!< ip del cliente
    unsigned short port; //!< puerto del cliente
    Client *next;
    Client *prev;
    Client(){
        next = NULL;
        prev = NULL;
        id = 0;
    }
    static Client *addClient(Client *l,Client *c){
        c->next = l;
        l->prev = c;
        return c;
    }
    static Client *removeClient(Client *l,Client *c){


    }

};


Client clients[MAX_CLIENTS];
Client *free_clients = NULL;
Client *conected_clients = NULL;
unsigned int next_id = 0;

char layrinth[400] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};
void error(const char *name,...) {
    printf("%s\n",name);
    exit(1);
}

int main(int argc,char **argv) {
    int sock = -1;
    sockaddr_in serverAddr;

    //creo una lista de clientes libres
    free_clients = &clients[0];
    for(int i=0;i<MAX_CLIENTS-1;i++){
        clients[i].next = &clients[i+1]; //basta con next
    }


#ifdef WINDOWS
    WSADATA wsaData = {0};
    int iResult = 0;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif

    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3030);

    if((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
        error("Error al crear al socket");
    }

    if(bind(sock,reinterpret_cast<sockaddr*>(&serverAddr),sizeof(serverAddr)) < 0) {
        closesocket(sock);
        error("Error al hacer bind");
    }

    //server loop
    bool done = false;
    sockaddr_in from;
    size_t from_len;
    while(!done) {
        //pdu del cliente
        pdu_client data_client;
        from_len = sizeof(from);
        /// \todo agregar select
        //recibimos los datos
        size_t rlen;
        if((rlen = recvfrom(sock,&data_client,sizeof(data_client),0,reinterpret_cast<sockaddr*>(&from),&from_len)) < 0){
            continue; //ignorar este paquete, esta malo
        }
        //ahora procesar paquetes
        if(data_client.id == 0){ //cliente nuevo
            if(free_clients != NULL){ //hay un cupo disponible??
                Client *c = free_clients;//saco un cliente
                free_clients->next = free_clients;
                c->id = ++next_id;
                c->ip.S_addr = from.sin_addr.S_addr;
                c->port = from.sin_port;
                //agregar un cliente disponible :P
                conected_clients = Client::addClient(conected_clients,c);
                //cliente conectado.
                printf("Cliente %s conectado\n",net_ntoa(c->ip));
            }else{ //no, no lo hay
                //rechazar!!!
                pdu_login login;
                login.type = EAT_Reject_User;
                login.client.id = 0; //fuiste rechazado
                from_len = sizeof(from);
                sendfrom(sock,&login,sizeof(login),0,reinterpret_cast<sockaddr*>(&from),&from_len)); //rechazado, por lento xD
            }
        }else{ //cliente ya conectado...

        }


    }



    closesocket(sock);
    return 0;
}
