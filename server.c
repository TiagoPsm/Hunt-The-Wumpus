
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>

#define N      5        
#define PORTA  51171
#define BUF    256

typedef struct {
    int r, c;
} Pos;


Pos wumpus, heroi, arma;
int temArma = 0;


int enviarLinha(SOCKET s, const char *txt) {
    char buf[BUF];
    int total, enviado, r;
    sprintf(buf, "%s\n", txt);
    total = (int)strlen(buf);
    enviado = 0;
    while (enviado < total) {
        r = send(s, buf + enviado, total - enviado, 0);
        if (r == SOCKET_ERROR) return -1;
        enviado += r;
    }
    return 0;
}

int recvLinha(SOCKET s, char *buf, int max) {
    int i = 0, r;
    char ch;
    while (i < max - 1) {
        r = recv(s, &ch, 1, 0);
        if (r <= 0) return -1;
        if (ch == '\n') break;
        if (ch != '\r') buf[i++] = ch;
    }
    buf[i] = '\0';
    return i;
}


int igual(Pos a, Pos b) {
    return a.r == b.r && a.c == b.c;
}

int adjacente(Pos a, Pos b) {
    return abs(a.r - b.r) + abs(a.c - b.c) == 1;
}

int mover(Pos *p, char dir) {
    int nr = p->r, nc = p->c;
    switch (dir) {
        case 'W': nr--; break;
        case 'S': nr++; break;
        case 'A': nc--; break;
        case 'D': nc++; break;
        default: return 0;
    }
    if (nr < 0 || nr >= N || nc < 0 || nc >= N) return 0;
    p->r = nr;
    p->c = nc;
    return 1;
}

void desenharMapa(const Pos *w, const Pos *h, const Pos *a) {
    int r, c;
    char ch;
    printf("\n    ");
    for (c = 0; c < N; c++) printf("%d ", c + 1);
    printf("\n");
    for (r = 0; r < N; r++) {
        printf("  %d ", r + 1);
        for (c = 0; c < N; c++) {
            ch = '.';
            if (a && a->r == r && a->c == c) ch = '!';
            if (w && w->r == r && w->c == c) ch = 'W';
            if (h && h->r == r && h->c == c) ch = 'H';
            printf("%c ", ch);
        }
        printf("\n");
    }
    printf("\n");
}

int enviarEstado(SOCKET cli, int status) {
    char msg[BUF];
    sprintf(msg, "STATE %d %d %d %d %d %d %d",
            heroi.r, heroi.c, arma.r, arma.c,
            temArma, adjacente(heroi, wumpus), status);
    return enviarLinha(cli, msg);
}

int main() {
    WSADATA winsocketsDados;
    SOCKET sock, clientSocket;
    struct sockaddr_in server, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int temp, valido, desconectou = 0;
    char linha[BUF];
    char tecla;

    srand((unsigned)time(NULL));

    temp = WSAStartup(MAKEWORD(2, 2), &winsocketsDados);
    if (temp != 0) {
        printf("WSAStartup falhou: %d\n", temp);
        return 1;
    }
    printf("WSAStartup carregado com sucesso\n");

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Erro ao criar o socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket criado com sucesso\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORTA);
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Erro ao associar o socket: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Bind realizado com sucesso\n");

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("Erro ao colocar o socket em estado de escuta: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Listen realizado com sucesso\n");

    printf("Aguardando o Heroi conectar na porta %d...\n", PORTA);
    clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        printf("Erro ao aceitar a conexao: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Heroi conectado!\n");

    do {
        wumpus.r = rand() % N; wumpus.c = rand() % N;
        heroi.r  = rand() % N; heroi.c  = rand() % N;
        arma.r   = rand() % N; arma.c   = rand() % N;
    } while (igual(wumpus, heroi) || igual(wumpus, arma) ||
             igual(heroi, arma) || adjacente(wumpus, heroi));

    printf("\n=========== HUNT THE WUMPUS - VOCE E O WUMPUS ===========\n");
    printf("Mate o heroi entrando na casa dele ANTES que ele pegue a arma!\n");
    printf("Movimentos: W=cima  S=baixo  A=esquerda  D=direita\n");
    printf("Legenda: W=voce  !=arma\n");

    while (1) {
        printf("\n--- Turno do Heroi (aguardando...) ---\n");
        do {
            if (enviarEstado(clientSocket, 0) < 0 ||
                recvLinha(clientSocket, linha, BUF) < 0) {
                desconectou = 1;
                break;
            }
            tecla = 0;
            if (strncmp(linha, "MOVE ", 5) == 0) tecla = (char)toupper((unsigned char)linha[5]);
            valido = mover(&heroi, tecla);
        } while (!valido);
        if (desconectou) break;

        if (!temArma && igual(heroi, arma)) {
            temArma = 1;
            printf("\n*** O HEROI PEGOU A ARMA! Fuja dele! ***\n");
        }

        if (igual(heroi, wumpus)) {
            if (temArma) {
                printf("\nO heroi entrou na sua casa armado... VOCE MORREU! Heroi venceu.\n");
                enviarEstado(clientSocket, 2);
            } else {
                printf("\nO heroi entrou na sua casa sem arma e morreu! VOCE VENCEU!\n");
                enviarEstado(clientSocket, 3);
            }
            break;
        }

        if (enviarEstado(clientSocket, 1) < 0) { desconectou = 1; break; }

        printf("\n--- Seu turno (Wumpus) ---\n");
        desenharMapa(&wumpus, NULL, temArma ? NULL : &arma);
        if (temArma) printf("ATENCAO: o heroi esta ARMADO!\n");
        if (adjacente(heroi, wumpus))
            printf(">> Voce sente um aroma de MEL... o heroi esta numa casa vizinha!\n");

        do {
            printf("Seu movimento (W/A/S/D): ");
            if (fgets(linha, sizeof(linha), stdin) == NULL) { desconectou = 1; break; }
            tecla = (char)toupper((unsigned char)linha[0]);
            valido = mover(&wumpus, tecla);
            if (!valido) printf("Movimento invalido!\n");
        } while (!valido);
        if (desconectou) break;

        if (igual(wumpus, heroi)) {
            if (temArma) {
                printf("\nVoce atacou o heroi, mas ele estava armado... VOCE MORREU!\n");
                enviarEstado(clientSocket, 2);
            } else {
                printf("\nVOCE PEGOU O HEROI! VOCE VENCEU!\n");
                enviarEstado(clientSocket, 3);
            }
            break;
        }
      
    }

    if (desconectou) printf("\nConexao encerrada pelo outro jogador.\n");

    printf("\nPressione ENTER para sair...");
    getchar();

    closesocket(clientSocket);
    closesocket(sock);
    WSACleanup();
    return 0;
}
