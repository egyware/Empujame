#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <Common.h>
#include <DebugDraw.h>
#include <Box2D/Box2D.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <Timer.h>

#define FREE_SURFACE(x) if(x) SDL_FreeSurface(x)

SDL_Surface *letras = NULL;
bool sendPing = false;
bool done = false;
unsigned int latency_init = 0;
unsigned int my_id = 0;
unsigned int paquetes_recibidos = 0;

void error(const char *name,...) {
    fflush(stdout); //flush sdtdout
    fprintf(stderr,"%s\n",name);
    exit(1);
}

void toExit() {
    SDL_Quit();
#ifdef WINDOWS
    WSACleanup();
#endif
}

void ServidorNoResponde(Timer<unsigned int> *timer) {
    unsigned int sock = *timer->userData;
    if(sendPing){
        //no ha llegado nada, es
        printf("\n>El Servidor no responde (Timeout)");
        done = true; //asi el juego termina
    }else{
        //revisar si el server sigue vivo
        pdu_data_ping ping;
        ping.tipo = pdu_ping;
        ping.id = my_id;
        printf("> Mi id 2 %d\n",my_id);
        ping.serverRequest = false; //lol, no puedo hacer este tipo de peticiones :P
        send(sock,reinterpret_cast<char*>(&ping),sizeof(ping),0);
        timer->setTimer(5000);//5 seg
        sendPing = true;
    }

}

void BuscarPartida( unsigned int socket) {
    hostent* Dir = gethostbyname(NULL);
    in_addr ip;
    if (Dir != NULL) {
        if (4 == Dir->h_length) { //IPv4
            ip.S_un.S_un_b.s_b1 = Dir->h_addr[0];
            ip.S_un.S_un_b.s_b2 = Dir->h_addr[1];
            ip.S_un.S_un_b.s_b3 = Dir->h_addr[2];
            ip.S_un.S_un_b.s_b4 = Dir->h_addr[3];
            ip.s_addr = ip.s_addr | htonl(0x000000FF); //direccion broadcast

            struct sockaddr_in s;
            memset(&s, 0, sizeof(struct sockaddr_in));
            s.sin_family = AF_INET;
            s.sin_port = htons(3030);
            s.sin_addr.s_addr = ip.s_addr;
            pdu_data_search Busqueda;
            Busqueda.tipo = pdu_search;
            sendto(socket,reinterpret_cast<char*>(&Busqueda),sizeof(Busqueda),0,reinterpret_cast<sockaddr*>(&s),sizeof(s));//envio una pdu
            unsigned int t1=getTicks();
            unsigned int t2=t1;
            pdu_data_info infor;
            int s_len = sizeof(s);
            fd_set timeout_search;
            timeval timeout;
            while(t2 - t1 < 5000) {
                timeout.tv_sec = 0;
                timeout.tv_usec = 500000; //5 milli
                FD_ZERO(&timeout_search);
                FD_SET(socket,&timeout_search);
                if (select(socket+1,&timeout_search,NULL,NULL,&timeout)>0) {
                    if(FD_ISSET(socket,&timeout_search)){
                        recvfrom(socket,reinterpret_cast<char*>(&infor),sizeof(infor),0,reinterpret_cast<sockaddr*>(&s), &s_len);
                        paquetes_recibidos++;
                        printf("Nombre: %s\n Max players: %d\n Conectados: %d\n Ip: %s\n",infor.name,infor.max_players,infor.players,inet_ntoa(s.sin_addr));
                    }
                }
                t2=getTicks();
            }
        }
    }

}

void ImpNick(SDL_Surface *screen,int x, int y,char *format,...) {
    SDL_Rect r= {0,0,32,32};
    SDL_Rect q= {x,y,0,0};
    char result[1024]; //1kb no creo que haya problemas :)
    int a,b;
    va_list list; //codigo hecho por Viktor, pero le agregue argumentos variables :)
    va_start(list,format);
    vsprintf(result,format,list);
    va_end(list);
    for (int i=0; result[i]!=0; i++) {
        int letra=result[i];
        b=letra/16;
        a=letra%16;
        r.x=a*32;
        r.y=b*32;
        SDL_BlitSurface(letras,&r,screen,&q);
        q.x=q.x+32;
    }
}

#undef main
int main() {

    //inicialización
#ifdef WINDOWS
    WSADATA wsaData = {0};
    int iResult = 0;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif
    atexit(toExit);

    //variables del socket
    unsigned int sock = 0; //no puede ser 0 en ningun momento :P
    sockaddr_in serverAddr;
    unsigned int last_turn = 0;
    Timer<unsigned int> serverRespuesta;
    serverRespuesta.userData = &sock;
    serverRespuesta.setCallBack(ServidorNoResponde);

    //variables del juego
    SDL_Surface *screen;
    SDL_Surface *j[4]= {NULL,NULL,NULL,NULL};
    Vector dir;
    b2World mundo(b2Vec2_zero); //creamos el mundo
#ifdef DEBUG
    DebugDraw dd;
    dd.AppendFlags(~0);
    mundo.SetDebugDraw(&dd);
#endif
    b2CircleShape circulo;
    circulo.m_radius = 30*SCALE;

    b2Body *b[MAX_CLIENTS];
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circulo;
    fixtureDef.density = 1.0f;

    //cuerpo 1
    bodyDef.position.Set(3,3);
    b[0] = mundo.CreateBody(&bodyDef);
    b[0]->CreateFixture(&fixtureDef);
    //cuerpo 2
    bodyDef.position.Set(3,3);
    b[1] = mundo.CreateBody(&bodyDef);
    b[1]->CreateFixture(&fixtureDef);
    //cuerpo 3
    bodyDef.position.Set(3,3);
    b[2] = mundo.CreateBody(&bodyDef);
    b[2]->CreateFixture(&fixtureDef);
    //cuerpo 4
    bodyDef.position.Set(3,3);
    b[3] = mundo.CreateBody(&bodyDef);
    b[3]->CreateFixture(&fixtureDef);

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

    // ** bordes **
    b2BodyDef defb;
    b2Body *borde = mundo.CreateBody(&defb);
    b2EdgeShape shape;

    shape.Set(b2Vec2(0.0f, 0.0f), b2Vec2(640*SCALE, 0.0f));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 480*SCALE));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(640*SCALE, 0.0f), b2Vec2(640*SCALE, 480*SCALE));
    borde->CreateFixture(&shape, 0.0f);

    shape.Set(b2Vec2(0.0f, 480*SCALE), b2Vec2(640*SCALE, 480*SCALE));
    borde->CreateFixture(&shape, 0.0f);



    char nick[128];
    char IP[16];
    bool caida = false;
    bool ganador = false;
    //nos conectamos al mini lobby
    //abrimos el socket
    if ((sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
        error("Error al crear al socket");
    }
    pdu_data_client cliente;
    pdu_data data;
    cliente.tipo = pdu_client;
    printf("___________                        __                     ._.\n");
    printf("\\_   _____/ _____ ______  __ __   |__|____    _____   ____| |\n");
    printf(" |    __)_ /     \\|     \\|  |  \\  |  \\__  \\  /     \\_/ __ \\ |\n");
    printf(" |        \\  Y Y  \\  |_> >  |  /  |  |/ __ \\|  Y Y  \\  ___/\\|\n");
    printf("/_______  /__|_|  /   __/|____/\\__|  (____  / __|_| / \\___> _\n");
    printf("        \\/      \\/|__|        \\______|    \\/      \\/     \\/\\/\n");
    printf("Bienvenido al juego!\n");
    printf("Porfavor ingresa tu nickname: ");
    scanf("%s",nick);
    printf("tu nick es: %s \n",nick);//se guarda el nick en una variable global
    printf("\n---Zona de concexion---\n");
    BuscarPartida(sock);
    printf("\n Ingrese la IP del servidor: ");
    scanf("%s",IP);
    cliente.id = 0; //quiero loggearme
    //seteamos las direcciones
    memset(&serverAddr,0,sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3030);
    serverAddr.sin_addr.s_addr = inet_addr(IP);



    //zona de login
    fd_set timeout_login;
    timeval timeout;
    bool isLogin = false;
    pdu_data_login login;
    login.tipo = pdu_login;
    login.id = 0;
    sprintf(login.nick,"%s",nick);
    printf(">Conectarse al mini lobby\n");
    for (int i = 0; i < TRY_AGAIN_LOGIN; i++) {
        if (i) {
            printf("\n>Conectandose al servidor. Intento %d...",i+1);
        } else {
            printf("\n>Conectandose al servidor...");

        }
        FD_ZERO(&timeout_login);
        FD_SET(sock,&timeout_login);
        timeout.tv_sec = 5; //5 segundos
        timeout.tv_usec = 0;
        sendto(sock,reinterpret_cast<char*>(&login),sizeof(login),0,reinterpret_cast<sockaddr*>(&serverAddr),sizeof(serverAddr));//envio una pdu
        //espero recibir un dato
        if (select(sock+1,&timeout_login,NULL,NULL,&timeout)>0) {
            if (FD_ISSET(sock,&timeout_login)) {
                pdu_data data;
                recv(sock,reinterpret_cast<char*>(&data),sizeof(data),0);
                paquetes_recibidos++;
                if (pdu_login == data.tipo) { //somos rechazados?
                    printf("\n>Conectado ID otorgada %d",data.login.id);
                    cliente.id = data.login.id;
                    //parche :c
                    my_id = data.login.id;
                    isLogin = true;
                    break;
                } else {
                    printf("\n>No se pudo conectar :(");
                    closesocket(sock);//salimos al tiro
                    return 0;
                }
            }
        }
        Wait(1000); //esperar 1 segundo
    }
    if (!isLogin) {
        error("\nEl servidor no responde...");
        return 1;
    }
    if (!hacerNoBloqueante(sock)) {
        printf("\n>No se pudo hacer no bloqueante :(");
        closesocket(sock);
        return 1;
    }
    //Hacemos que send envie por defecto a una sola direccion
    connect(sock,reinterpret_cast<sockaddr*>(&serverAddr),sizeof(serverAddr));

    printf("\n>Cargando el juego...");
    //si la connecion fue exitosa
    screen = SDL_SetVideoMode(640,480,24,SDL_HWSURFACE|SDL_DOUBLEBUF);
#ifdef DEBUG
    dd.paint(screen);
#endif
    SDL_WM_SetCaption("Juego",NULL);

    SDL_Surface *space; //in space!!
    SDL_Surface *plataforma;
    SDL_Surface *winner;
    SDL_Surface *perdiste;
    SDL_Surface *ping_img;
    j[0] = IMG_Load("victor.png");
    j[1] = IMG_Load("mariana.png");
    j[2] = IMG_Load("egy.png");
    j[3] = IMG_Load("hans.png");

    space = IMG_Load("space.png");
    letras = IMG_Load("letras.bmp");
    plataforma= IMG_Load("circulito.png");
    winner = IMG_Load("winner.png");
    perdiste = IMG_Load("gg.png");
    ping_img = IMG_Load("ping.png");

    for (int i=0; i<MAX_CLIENTS; i++) {
        SDL_SetColorKey(j[i],SDL_SRCCOLORKEY,SDL_MapRGB(j[i]->format,255,0,255));
    }
    SDL_SetColorKey(ping_img,SDL_SRCCOLORKEY,SDL_MapRGB(ping_img->format,255,0,255));
    SDL_SetColorKey(letras,SDL_SRCCOLORKEY,SDL_MapRGB(letras->format,255,0,255));
    SDL_SetColorKey(plataforma,SDL_SRCCOLORKEY,SDL_MapRGB(plataforma->format,255,0,255));
    SDL_SetColorKey(perdiste,SDL_SRCCOLORKEY,SDL_MapRGB(perdiste->format,255,0,255));
    SDL_SetColorKey(winner,SDL_SRCCOLORKEY,SDL_MapRGB(winner->format,255,0,255));
    printf("listo");

    //control de FPS
    unsigned int t_init = SDL_GetTicks(); //tomo el tiempo inicial
    unsigned int t_delta = 0; //diferencia de tiempos
    unsigned int t_end = 0;  //tiempo final
    bool rest = true; //logro descanzar? si es asi sirve para indicar que se puede pintar sin ningún problema
    bool serverOff = false; //servidor desconectado
    //game loop, repite hasta que done sea true
    while (!done) {
        // *** procesamiento de eventos ***
        bool sendChangeData = false;
        SDL_Event evento;
        if (SDL_PollEvent(&evento)) {
            switch (evento.type) {
            case SDL_QUIT: {
                done = true;
            }
            break;
            case SDL_KEYDOWN: {
                if (evento.key.keysym.sym == SDLK_LEFT) {
                    dir.x = -1;
                    sendChangeData = true;
                } else if (evento.key.keysym.sym == SDLK_RIGHT) {
                    dir.x = 1;
                    sendChangeData = true;
                }

                if (evento.key.keysym.sym == SDLK_UP) {
                    dir.y = -1;
                    sendChangeData = true;
                } else if (evento.key.keysym.sym == SDLK_DOWN) {
                    dir.y = 1;
                    sendChangeData = true;
                } else if(evento.key.keysym.sym == SDLK_q) {
                    done=true;
                }

            }
            break;
            case SDL_KEYUP: {
                if (evento.key.keysym.sym == SDLK_LEFT || evento.key.keysym.sym == SDLK_RIGHT) {
                    dir.x = 0;
                }
                if (evento.key.keysym.sym == SDLK_UP || evento.key.keysym.sym == SDLK_DOWN) {
                    dir.y = 0;
                }
                sendChangeData = true;
            }
            break;
            default: //caso por defecto, no hacer nada
                break;
            };
        } // fin eventosn
        // *** logica ***
        //envio mis datos :)
        if (sendChangeData && !serverOff) { //si el servidor está apagado
            cliente.direccion = dir;
            send(sock,reinterpret_cast<char*>(&cliente),sizeof(cliente),0);
        }

        //recibo que es lo que paso en el server
        if (recv(sock,reinterpret_cast<char*>(&data),sizeof(data),0)>0) { //espero recibir un dato
            paquetes_recibidos++;
            switch (data.tipo) {
            case pdu_server: {
                if(last_turn > data.server.turno){
                    //descartar
                    printf("\n> Paquete descartado %d(<%d)",data.server.turno,last_turn);
                }else{
                    last_turn = data.server.turno;
                for (int i=0; i<4; i++) { //recibir las posiciones
                    b2Vec2 v(data.server.velocidad[i].x,data.server.velocidad[i].y);
                    b2Vec2 p(data.server.posiciones[i].x,data.server.posiciones[i].y);
                    b[i]->SetTransform(p,0);
                    b[i]->SetLinearVelocity(v);
                }
                if(data.server.caida) {
                    caida = true;
                    serverOff = true; //ahora le da lo mismo el ping
                    //se cayo :(
                }
                }
            }
            break;
            case pdu_winner:{
                printf("\n> Gane!!");
                ganador = true; //gane :)
                serverOff = true; //el servidor despues de esto nos desconecta
            }
            break;
            case pdu_disconnect:{
                if(cliente.id == data.disconnect.id){ //las ID coinciden quieren que me vaya :/
                    serverOff = true;
                }else{ //ufff casi me voy
                    //deberia buscar los clientes por id, pero que flojera
                    printf("\n> El cliente '%d' se %s",data.disconnect.id,(Irse == data.disconnect.razon)?"fue":"desconecto");
                }
            }break;
            case pdu_login:{
                printf("\n> El cliente '%s(%d)' se ha conectado",data.login.nick,data.login.id);
            }break;
            case pdu_ping:{
                if(data.ping.serverRequest){//responder in ping
                    pdu_data_ping ping;
                    ping.tipo = pdu_ping;
                    ping.id = my_id;
                    printf("> Mi id %d",my_id);
                    ping.serverRequest = false; //lol, no puedo hacer este tipo de peticiones :P
                    send(sock,reinterpret_cast<char*>(&ping),sizeof(ping),0);
                }else{
                    printf("\n> Ping resuelto");
                }
            }break;
            }
            sendPing = false; //nos contentamos igual, si cualquier dato, inclusive si se pierde un ping
            latency_init = t_end;
            serverRespuesta.setTimer(2500); //2.5 seg reseteamos el timer a 0, nos llego un dato
            printf("\n");
        }//recibio un dato?
        printf("%5u ms\b\b\b\b\b\b\b\b",t_end - latency_init);


        mundo.Step(1.0f/30.0f,6,2);
        mundo.ClearForces();


        if (rest) { //descanzo? si entonces pintamos :P
            // *** pintado ***
            SDL_Rect plat; //para posicionar la platadorma
            plat.x=(screen->w - plataforma->w)/2;
            plat.y=(screen->h - plataforma->h)/2;

            SDL_BlitSurface(space,NULL,screen,NULL); //borramos todo en la pantalla
            SDL_BlitSurface(plataforma,NULL,screen,&plat);//pintando la plataforma
            float32 pos[MAX_CLIENTS];
            unsigned int ids[MAX_CLIENTS];
            for (int c = 0; c<MAX_CLIENTS; c++) { //recibir las posiciones
                pos[c] = b[c]->GetPosition().y;
                ids[c] = c;
            }
            //ordenar
            for(int i=0; i<MAX_CLIENTS; i++) {
                float32 min = pos[i];
                for(int j=i; j<MAX_CLIENTS; j++) {
                    if(min > pos[j]) {
                        pos[i] = pos[j];
                        pos[j] = min;
                        min = pos[j];
                        unsigned int id = ids[i];
                        ids[i] = ids[j];
                        ids[j] = id;
                    }
                }
            }
            for(int c=0; c<MAX_CLIENTS; c++) {
                unsigned int i = ids[c];
                SDL_Rect pos;
                b2Vec2 p = b[i]->GetPosition();
                pos.x = static_cast<Sint16>(p.x * INV_SCALE)-j[i]->w/2;
                pos.y = static_cast<Sint16>(p.y * INV_SCALE)-3*j[i]->h/4;
                SDL_BlitSurface(j[i%4],NULL,screen,&pos);//para que pinte mas 4 clientes
            }

            ImpNick(screen,0,0,nick);
#ifdef DEBUG
            mundo.DrawDebugData();
#endif
            if(caida) {
                SDL_Rect pos;
                pos.x= 170;
                pos.y= 185;
                SDL_BlitSurface(perdiste,NULL,screen,&pos);
            }
            if(ganador){
                SDL_Rect pos;
                pos.x = (screen->w - winner->w)/2;
                pos.y = (screen->h - winner->h)/2;
                SDL_BlitSurface(winner,NULL,screen,&pos);
            }
            if(sendPing){
                SDL_Rect pos;
                pos.x = (screen->w - ping_img->w);
                pos.y = 0;
                SDL_BlitSurface(ping_img,NULL,screen,&pos);
            }
            SDL_Flip(screen);

        }
        //control de FPS
        t_end = SDL_GetTicks();
        t_delta = t_end - t_init;
        if(!serverOff) //El servidor se desconecto
            serverRespuesta.update(t_delta);
        if (t_delta < 30) {
            Wait(30-t_delta);
            rest = true;
        } else {
            rest = false;
        }
        t_init = t_end;

    }
    printf("\n> Paquetes recibidos %u",paquetes_recibidos);

    pdu_data_disconnect desconectar;
    desconectar.id=cliente.id;
    desconectar.razon= Irse;
    desconectar.tipo = pdu_disconnect;
    sendto(sock,reinterpret_cast<char*>(&desconectar),sizeof(desconectar),0,reinterpret_cast<sockaddr*>(&serverAddr),sizeof(serverAddr));
    closesocket(sock);

    for (int i=0; i<4; i++) {
        FREE_SURFACE(j[i]);
    }
    FREE_SURFACE(space);
    FREE_SURFACE(winner);
    FREE_SURFACE(letras);
    FREE_SURFACE(perdiste);
    FREE_SURFACE(ping_img);

    return 0;
}

