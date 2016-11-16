#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <Common.h>
#include <Timer.h>
#include <cmath>
#include <Box2D/Box2D.h>
#include <Listener.h>

//#include <SDL/SDL_timer.h>

#define PI 3.1415926536


Client clients[MAX_CLIENTS];
char name_partida[64];
unsigned int next_id = 0;
unsigned int max_jugadores=3;
unsigned int jugadores=max_jugadores;
unsigned int turno = 0;
unsigned int paquetes_enviados = 0;
Listener listener;

sockaddr_in to;

void error(const char *name,...) {
    printf("%s\n",name);
    exit(1);
}

//si desean podemos declarar ciertas variables como globales
void sendData(Timer<unsigned int> *timer) {
    int sock = *timer->userData;
    pdu_data_server data;
    data.tipo = pdu_server;
    data.caida = 0; //nadie se cayo..
    data.choque = 0; //nadie choco con nadie
    data.turno = ++turno;
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].id!=0) {
            b2Vec2 p = clients[i].body->GetPosition();
            b2Vec2 v = clients[i].body->GetLinearVelocity();
            data.posiciones[i].x = p.x;
            data.posiciones[i].y = p.y;
            data.velocidad[i].x = v.x;
            data.velocidad[i].y = v.y;
            //printf("%f  %f \n", p.x,p.y);
        } else {
            data.posiciones[i].x = (40+i*150)*SCALE;
            data.posiciones[i].y = 440*SCALE;
            data.velocidad[i].x = 0;
            data.velocidad[i].y = 0;
        }
    }
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].id != 0) {
            to.sin_addr = clients[i].ip;
            to.sin_port = clients[i].port;
            if(clients[i].caida) { //caida
                data.caida = true;
            } else {
                data.caida = false;
            }
            sendto(sock,reinterpret_cast<char*>(&data),sizeof(data),0,
                   reinterpret_cast<sockaddr*>(&to), sizeof(to)); //
            paquetes_enviados++;

        }
    }
    if(timer)
        timer->setTimer(50);
}
void revisarTimeout(unsigned int sock) {
    pdu_data_ping ping;
    ping.tipo = pdu_ping;
    ping.serverRequest = true;
    unsigned int now = getTicks();
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].id!=0) {
            unsigned int delta = now - clients[i].last_data;
            //printf("Time %d %u\n",i,delta);
            if(delta > 5000 && delta < 10000) {
                to.sin_addr = clients[i].ip;
                to.sin_port = clients[i].port;
                sendto(sock,reinterpret_cast<char *>(&ping),sizeof(ping),0,reinterpret_cast<sockaddr*>(&to),sizeof(to));
                paquetes_enviados++;
            } else if(delta > 10000) {
                //desconectarlo
                jugadores++;
                printf("> Cliente '%s' timeout\n",clients[i].nick);
                clients[i].id = 0; //se fue este cliente
                //notificar a los demás
                pdu_data_disconnect dis;
                dis.tipo = pdu_disconnect;
                dis.razon = Timeout; //por timeout
                for(int c = 0; c < MAX_CLIENTS; c++) {
                    if(clients[c].id != 0) {
                        to.sin_addr = clients[c].ip;
                        to.sin_port = clients[c].port;
                        sendto(sock,reinterpret_cast<char*>(&dis),sizeof(dis),0,
                               reinterpret_cast<sockaddr*>(&to),sizeof(to)); //aceptado
                        paquetes_enviados++;
                    }
                }
                printf("> Se fue el cliente '%s'\n",clients[i].nick);
                b2Body *body = clients[i].body; //al cambiar su posición, inmediatamente pierde :P
                b2Vec2 p((100+i*100)*SCALE,440*SCALE);
                body->SetTransform(p,0);
                p = body->GetPosition();
            }
        }
    }
}
bool processLogin(unsigned int sock,pdu_data_login &login,sockaddr_in &from,b2World &mundo) {
    //definicion del circulo
    b2CircleShape circulo;
    circulo.m_radius = 30*SCALE;
    b2FixtureDef circuloFixture;
    circuloFixture.shape = &circulo;
    circuloFixture.density = 1.0f;
    circuloFixture.restitution = 1.0f;

    //cliente nuevo
    int i;
    for (i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].id == 0) {
            break; //lo encontramos :D!!!
        }
    }
    if (i<MAX_CLIENTS) {
        //revisamos si el for encontro un espacio disponible
        clients[i].id = i+1;
        clients[i].ip = from.sin_addr;
        clients[i].port = from.sin_port;
        // sprintf(clients[i].nick,data_client.nick);
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.fixedRotation = true;
        //agregar un cliente disponible :P
        bodyDef.position.x = (320+2*cos(2*PI/MAX_CLIENTS*i))*SCALE;
        bodyDef.position.y = (240+2*sin(2*PI/MAX_CLIENTS*i))*SCALE;
        bodyDef.userData = &clients[i];
        clients[i].body = mundo.CreateBody(&bodyDef);
        clients[i].body->CreateFixture(&circuloFixture);
        clients[i].last_data = getTicks(); //anoto su ultimo tiempo :)
        sprintf(clients[i].nick,"%s",login.nick);

        //cliente conectado.
        printf("Cliente %s(%s) conectado\n",clients[i].nick,inet_ntoa(clients[i].ip));
        //avisar que estas conectado :), usamos los mismos datos, pero con ip asignada
        login.id = i+1;
        //enviar al jugador conectado
        sendto(sock,reinterpret_cast<char*>(&login),sizeof(login),0,
               reinterpret_cast<sockaddr*>(&from),sizeof(from)); //aceptado
        paquetes_enviados++;
        //luego a los demás
        for(int c = 0; c < MAX_CLIENTS; c++) {
            if(c != i && clients[c].id != 0) {
                from.sin_addr = clients[c].ip;
                from.sin_port = clients[c].port;
                sendto(sock,reinterpret_cast<char*>(&login),sizeof(login),0,
                       reinterpret_cast<sockaddr*>(&from),sizeof(from)); //aceptado
                paquetes_enviados++;
            }
        }
        jugadores--;
    } else { //no, no lo hay
        //rechazar!!!
        clients[i].id = 0; //fuiste rechazado
        sendto(sock,reinterpret_cast<char*>(&login),sizeof(login),0,
               reinterpret_cast<sockaddr*>(&from),sizeof(from)); //rechazado, por lento xD
        paquetes_enviados++;
    }
    if (0 == jugadores) {
        return false;
    } else {
        return true;
    }
}

void responderPing(unsigned int sock,unsigned id,sockaddr_in &from) {
    pdu_data_ping ping;
    ping.tipo = pdu_ping;
    ping.serverRequest = false; //no es una peticion del servidor
    unsigned int i = id - 1;
    if(i >=0 && i < MAX_CLIENTS) { //pero solo responder si existe ese dato
        if(clients[i].id == id) {
            clients[i].last_data = getTicks();
            //respondimos a la misma direccion
            sendto(sock,reinterpret_cast<char *>(&ping),sizeof(ping),0,
                   reinterpret_cast<sockaddr*>(&from), sizeof(from));
            paquetes_enviados++;
        }
    }//si no? quien me envio datos?? xD



}

#include <unistd.h>


int main(int argc,char **argv) {
    printf("___________                        __                     ._.\n");
    printf("\\_   _____/ _____ ______  __ __   |__|____    _____   ____| |\n");
    printf(" |    __)_ /     \\|     \\|  |  \\  |  \\__  \\  /     \\_/ __ \\ |\n");
    printf(" |        \\  Y Y  \\  |_> >  |  /  |  |/ __ \\|  Y Y  \\  ___/\\|\n");
    printf("/_______  /__|_|  /   __/|____/\\__|  (____  / __|_| / \\___> _\n");
    printf("        \\/      \\/|__|        \\______|    \\/      \\/     \\/\\/\n");
    printf("Bienvenido al servidor!\n");
#ifdef WINDOWS
    WSADATA wsaData = {0};
    int iResult = 0;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    typedef int socklen_t;
#endif
//*******************************************
    hostent* Dir = gethostbyname(NULL);
    if (Dir != NULL) {
        if (4 == Dir->h_length) { //IPv4
            in_addr ip;
            ip.S_un.S_un_b.s_b1 = Dir->h_addr[0];
            ip.S_un.S_un_b.s_b2 = Dir->h_addr[1];
            ip.S_un.S_un_b.s_b3 = Dir->h_addr[2];
            ip.S_un.S_un_b.s_b4 = Dir->h_addr[3];
            printf(">Tu IP: %s\n", inet_ntoa(ip));

        }
    }
//******************************************
    printf("> Cuantos jugadores (MAX %d): ",MAX_CLIENTS);
    scanf("%d",&max_jugadores);
    if(max_jugadores > MAX_CLIENTS) {
        max_jugadores = MAX_CLIENTS;
    }
    jugadores = max_jugadores;
    printf("> Jugadores %d \n",max_jugadores);
    printf("> Nombre de la partida: ");
    scanf("%s",name_partida);
    //variables del servidor
    unsigned int sock = 0;
    sockaddr_in serverAddr;

    //variables del juego
    Timer <unsigned int> sender;
    b2World mundo(b2Vec2_zero);
    mundo.SetContactListener(&listener);

    b2BodyDef defb;
    b2Body *borde = mundo.CreateBody(&defb);
    b2EdgeShape shape;

    shape.Set(b2Vec2(0.0f, 0.0f), b2Vec2(6.4f, 0.0f));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 4.8f));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(6.4f, 0.0f), b2Vec2(6.4f, 4.8f));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(0.0f, 4.8f), b2Vec2(6.4f, 4.8f));
    borde->CreateFixture(&shape, 0.0f);

    //sensor
    b2BodyDef plataformaDef;
    plataformaDef.position.Set(320*SCALE,240*SCALE);
    plataformaDef.type = b2_staticBody;

    b2Vec2 vertices[8];
    for(int i=0; i<8; i++) {
        vertices[i].x = 210*cos(M_PI_4*0.5f+i*M_PI_4)*SCALE;
        vertices[i].y = 100*sin(M_PI_4*0.5f+i*M_PI_4)*SCALE;
    }
    b2PolygonShape circuloPlataforma;
    circuloPlataforma.Set(vertices,8);

    b2FixtureDef fixtureCirculo;
    fixtureCirculo.isSensor = true;
    fixtureCirculo.shape = &circuloPlataforma;

    b2Body *sensor = mundo.CreateBody(&plataformaDef);
    sensor->CreateFixture(&fixtureCirculo);

    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3030);

    memset(&to,0,sizeof(to));
    to.sin_family = AF_INET;

    //abriendo el socket
    if ((sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
        error("Error al crear al socket");
    }

    if (bind(sock,reinterpret_cast<sockaddr*>(&serverAddr),sizeof(serverAddr)) < 0) {
        closesocket(sock);
        error("Error al hacer bind");
    }

    //server loop
    bool wait=true;
    bool done = false;
    pdu_data data;
    sockaddr_in from;
    socklen_t from_len;
    printf(">Esperando jugadores\n");
    // *** mini lobby ***
    fd_set lobby;
    while (wait) {
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; //500 milisegundos
        FD_ZERO(&lobby);
        FD_SET(sock,&lobby);
        /// \todo agregar select
        //recibimos los datos
        if(select(sock+1,&lobby,NULL,NULL,&timeout)) {
            if(FD_ISSET(sock,&lobby)) {
                from_len = sizeof(from);
                if (recvfrom(sock,reinterpret_cast<char*>(&data),sizeof(data),0,
                             reinterpret_cast<sockaddr*>(&from), &from_len)<0) {
                    continue; //ignorar este paquete, esta malo
                }
                //ahora procesar paquetes
                switch(data.tipo) {
                case pdu_ping: {
                    //responder sus ping .-.
                    responderPing(sock,data.ping.id,from);
                }
                break;
                case pdu_login: {
                    wait = processLogin(sock,data.login,from,mundo);
                    break;
                }
                case pdu_server: {
                    //no procesar aqui
                    break;
                }
                case pdu_client: {
                    //no procesar aqui
                    break;
                }
                case pdu_info: {
                    //vea pdu_search
                    break;
                }
                case pdu_disconnect: {
                    //se fue alguien, lo deje más simple (aunque nos pueden hackear)
                    unsigned int id = data.disconnect.id - 1;
                    if(id >= 0 && id <= MAX_CLIENTS) { //revisar bordes
                        if(clients[id].id - 1 == id) { //son iguales?
                            clients[id].id = 0; //se fue este cliente
                            jugadores++;
                            //notificar a los demás
                            data.disconnect.razon = Irse;
                            for(int i = 0; i < MAX_CLIENTS; i++) {
                                if(clients[i].id != 0) {
                                    from.sin_addr = clients[i].ip;
                                    from.sin_port = clients[i].port;
                                    sendto(sock,reinterpret_cast<char*>(&data.disconnect),sizeof(data.disconnect),0,
                                           reinterpret_cast<sockaddr*>(&from),sizeof(from)); //aceptado
                                    paquetes_enviados++;
                                }
                            }
                            printf("> Se desconecto '%s'\n",clients[id].nick);
                        }
                    }
                    break;
                }
                case pdu_search: {
                    //alguien esta buscando una partida
                    pdu_data_info info;
                    info.tipo = pdu_info;
                    info.max_players = max_jugadores;
                    info.players = max_jugadores - jugadores;
                    sprintf(info.name,"%s",name_partida);
                    sendto(sock,reinterpret_cast<char *>(&info),sizeof(info),0,
                           reinterpret_cast<sockaddr*>(&from),sizeof(from));
                    paquetes_enviados++;
                    break;
                }
                default: //los demás pdu no se manejan
                    break;
                } //sin del switch
                continue;
            }
        }
        revisarTimeout(sock);
    }

    hacerNoBloqueante(sock);
    turno = 0;
//***Game loop *****
    jugadores = max_jugadores;
    /**/
    unsigned int t_init = getTicks();
    /**/
    unsigned int t_delta = 0;
    /**/
    unsigned int t_end = 0;
    /**/
    sender.setCallBack(sendData);
    /**/
    sender.userData = &sock;
    /**/
    sender.setTimer(50);
    while (!done) {
        //pdu del cliente
        if(recvfrom(sock,reinterpret_cast<char*>(&data),sizeof(data),0,reinterpret_cast<sockaddr*>(&from), &from_len)>0) {
            switch(data.tipo) {
            case pdu_client: {
                unsigned ide = data.client.id - 1;
                if(0<=ide && ide<MAX_CLIENTS) {
                    if(!clients[ide].caida) { //un cliente caido NO puede moverse :P
                        Vector dir = data.client.direccion;
                        clients[ide].direccion = dir;
                        b2Vec2 force;
                        force.x = dir.x*60*SCALE;
                        force.y = dir.y*60*SCALE;
                        clients[ide].body->SetLinearVelocity(force);
                        clients[ide].last_data = getTicks();
                    }
                }
            }
            break;
            case pdu_disconnect: { //se fue alguien
                unsigned int id = data.disconnect.id - 1;
                if(id >= 0 && id <= MAX_CLIENTS) { //revisar bordes
                    if(clients[id].id - 1 == id) { //son iguales?
                        if((clients[id].ip.s_addr == from.sin_addr.s_addr) && (clients[id].port == from.sin_port)) { //su ip:port es igual?
                            clients[id].id = 0; //se fue este cliente
                            //notificar a los demás
                            data.disconnect.razon = Irse;
                            for(int i = 0; i < MAX_CLIENTS; i++) {
                                if(clients[i].id != 0) {
                                    from.sin_addr = clients[i].ip;
                                    from.sin_port = clients[i].port;
                                    sendto(sock,reinterpret_cast<char*>(&data.disconnect),sizeof(data.disconnect),0,
                                           reinterpret_cast<sockaddr*>(&from),sizeof(from)); //aceptado
                                    paquetes_enviados++;
                                }
                            }
                            printf("> Se fue el cliente '%s'\n",clients[id].nick);
                            b2Body *body = clients[id].body;
                            b2Vec2 p((100+id*100)*SCALE,440*SCALE);
                            body->SetTransform(p,0);
                            p = body->GetPosition();
                        }

                    }
                }
            }
            case pdu_ping: {
                //responder sus ping .-.
                responderPing(sock,data.ping.id,from);
            }
            break;
            }//fin del switch
        }

        //La logica del juego 1313
        mundo.Step(1/30.0f,6,2);
        mundo.ClearForces();

        //ver si alguien se fue
//        revisarTimeout(sock);

        //ver si alguien gano :)
        if(jugadores == 1) {
            int id = 0;
            pdu_data_winner winner;
            pdu_data_disconnect discon;
            winner.tipo = pdu_winner;
            discon.tipo = pdu_disconnect;
            discon.razon = Irse;
            for(int i=0; i<MAX_CLIENTS; i++) {
                if(clients[i].id != 0 && !clients[i].caida) {
                    id = i; //encontrado
                    break;
                }
            }
            from.sin_addr = clients[id].ip;
            from.sin_port = clients[id].port;
            sendto(sock,reinterpret_cast<char*>(&winner),sizeof(winner),0,
                   reinterpret_cast<sockaddr*>(&from),sizeof(from)); //ganó!!!!
            paquetes_enviados++;
            sendData(&sender); //codigo parche, para enviar GG :3
            for(int i=0; i<MAX_CLIENTS; i++) {
                if(clients[i].id != 0) {
                    from.sin_addr = clients[id].ip;
                    from.sin_port = clients[id].port;
                    sendto(sock,reinterpret_cast<char*>(&discon),sizeof(discon),0,
                           reinterpret_cast<sockaddr*>(&from),sizeof(from)); //Ya me aburrieron, todos se van!! >:3
                    paquetes_enviados++;
                }
            }
            done = true; // listo, ahora cerramos el server :)

        }

        //control de velocidad
        t_end = getTicks();
        t_delta = t_end - t_init;
        sender.update(t_delta); //este timer funciona 20 veces por segundo :
        if(t_delta < 30) {
            Wait(30-t_delta); //hacer que funcione 33 veces por segundo
        }//si no estamos corto de tiempo D:
        t_init = t_end;
    }
    printf("> Paquetes enviados %u\n",paquetes_enviados);
    printf("> Gracias por jugar\n");



    closesocket(sock);
    return 0;
}
