
#include <winsock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define N      5
#define PORTA  51171
#define BUF    256

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

void desenharMapa(int hr, int hc, int ar, int ac, int temArma) {
    int r, c;
    char ch;
    printf("\n    ");
    for (c = 0; c < N; c++) printf("%d ", c + 1);
    printf("\n");
    for (r = 0; r < N; r++) {
        printf("  %d ", r + 1);
        for (c = 0; c < N; c++) {
            ch = '.';
            if (!temArma && r == ar && c == ac) ch = '!';
            if (r == hr && c == hc) ch = 'H';
            printf("%c ", ch);
        }
        printf("\n");
    }
    printf("\n");
}

int movimentoValido(int hr, int hc, char dir) {
    switch (dir) {
        case 'W': hr--; break;
        case 'S': hr++; break;
        case 'A': hc--; break;
        case 'D': hc++; break;
        default: return 0;
    }
    return (hr >= 0 && hr < N && hc >= 0 && hc < N);
}

int main() {
    WSADATA winsocketsDados;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char ip[64], linha[BUF], msg[BUF];
    int hr, hc, ar, ac, temArma, cheiro, status;
    int fim = 0, len;
    char tecla;

    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    }
    printf("WSAStartup carregado com sucesso\n");

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Erro ao criar o socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket criado com sucesso\n");

    printf("IP do servidor (ENTER = 127.0.0.1): ");
    if (fgets(ip, sizeof(ip), stdin) == NULL) ip[0] = '\0';
    len = (int)strlen(ip);
    while (len > 0 && (ip[len - 1] == '\n' || ip[len - 1] == '\r')) ip[--len] = '\0';
    if (len == 0) strcpy(ip, "127.0.0.1");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(PORTA);
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Erro ao conectar ao servidor: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    printf("Conectado ao servidor\n");

    printf("\n=========== HUNT THE WUMPUS - VOCE E O HEROI ===========\n");
    printf("Pegue a arma (!) e entre na casa do Wumpus para mata-lo.\n");
    printf("Se entrar na casa dele SEM a arma, voce morre!\n");
    printf("Movimentos: W=cima  S=baixo  A=esquerda  D=direita\n");
    printf("Legenda: H=voce  !=arma\n");

    while (!fim) {
        if (recvLinha(clientSocket, linha, BUF) < 0) {
            printf("\nConexao encerrada pelo servidor.\n");
            break;
        }
        if (sscanf(linha, "STATE %d %d %d %d %d %d %d",
                   &hr, &hc, &ar, &ac, &temArma, &cheiro, &status) != 7) {
            continue; 
        }

        desenharMapa(hr, hc, ar, ac, temArma);
        if (temArma) printf("Voce esta ARMADO! Encontre o Wumpus e entre na casa dele!\n");
        if (cheiro)  printf(">> Voce sente um CHEIRO DESAGRADAVEL... o Wumpus esta numa casa vizinha!\n");

        switch (status) {
            case 0: 
                do {
                    printf("Seu movimento (W/A/S/D): ");
                    if (fgets(linha, sizeof(linha), stdin) == NULL) { fim = 1; break; }
                    tecla = (char)toupper((unsigned char)linha[0]);
                    if (!movimentoValido(hr, hc, tecla)) {
                        printf("Movimento invalido!\n");
                        tecla = 0;
                    }
                } while (tecla == 0);
                if (fim) break;
                sprintf(msg, "MOVE %c", tecla);
                if (enviarLinha(clientSocket, msg) < 0) {
                    printf("Erro ao enviar dados: %d\n", WSAGetLastError());
                    fim = 1;
                }
                break;

            case 1: 
                printf("Aguardando o movimento do Wumpus...\n");
                break;

            case 2:
                printf("\n*** VOCE VENCEU! O Wumpus foi derrotado! ***\n");
                fim = 1;
                break;

            case 3:
                printf("\n*** VOCE MORREU! O Wumpus venceu. ***\n");
                fim = 1;
                break;
        }
    }

    printf("\nPressione ENTER para sair...");
    getchar();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
