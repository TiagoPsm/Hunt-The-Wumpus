# Hunt the Wumpus – 2 Jogadores (Sockets TCP em C)

Versão para dois jogadores do clássico *Hunt the Wumpus*, feita em C com **Winsock** (Windows). Um jogador é o **Herói** e o outro é o **Wumpus**. Eles se comunicam por sockets TCP, seguindo o conteúdo do Capítulo 2 (Processos e Threads – Comunicação entre processos) de *Sistemas Operacionais Modernos*, de Andrew S. Tanenbaum.

## Regras

O jogo acontece em um mapa **5x5**. O Wumpus, o Herói e a arma nascem em casas aleatórias e diferentes (o Herói nunca nasce ao lado do Wumpus). Os jogadores se movem **alternadamente**, e o Herói começa.

| Situação | Resultado |
|---|---|
| Herói entra na casa do Wumpus **com** a arma | Herói vence |
| Herói entra na casa do Wumpus **sem** a arma | Herói morre, Wumpus vence |
| Wumpus entra na casa do Herói **sem** a arma | Wumpus vence |
| Wumpus entra na casa do Herói **já armado** | Wumpus morre, Herói vence |

**Percepções** (casas adjacentes = cima, baixo, esquerda e direita):
- Herói ao lado do Wumpus sente um **cheiro desagradável**.
- Wumpus ao lado do Herói sente um **aroma de mel**.

**Informação de cada jogador:** os dois veem a posição da arma e a própria posição. Nenhum vê o outro; só o percebem pelo cheiro e pelo mel. Quando o Herói pega a arma, o Wumpus é avisado.

**Controles:** `W` (cima), `S` (baixo), `A` (esquerda), `D` (direita) + ENTER.

**Legenda do mapa:** `H` = Herói, `W` = Wumpus, `!` = arma, `.` = casa vazia.

## Arquitetura

- **`server.c`**: roda o jogador **Wumpus**. Guarda todo o estado do jogo (posições, arma, regras de vitória) e envia ao cliente apenas o que o Herói pode saber.
- **`client.c`**: roda o jogador **Herói**. Mostra o mapa, lê o movimento e o envia ao servidor.

Fluxo, como nos slides da disciplina:

| Servidor | Cliente |
|---|---|
| `WSAStartup` | `WSAStartup` |
| `socket` | `socket` |
| `bind` | |
| `listen` | |
| `accept` | `connect` |
| `send` / `recv` | `send` / `recv` |
| `closesocket` + `WSACleanup` | `closesocket` + `WSACleanup` |

### Protocolo

As mensagens são linhas de texto terminadas em `\n`, sobre TCP (porta **27015**).

```
Servidor -> Cliente:  STATE <heroiLinha> <heroiColuna> <armaLinha> <armaColuna> <temArma> <cheiro> <status>
Cliente  -> Servidor: MOVE <W|A|S|D>
```

`status`: `0` = sua vez, `1` = aguarde o Wumpus, `2` = Herói venceu, `3` = Herói perdeu.

## Requisitos

- Windows
- Compilador C com suporte ao Winsock (MinGW/TDM-GCC, como o do **Dev-C++ 5.11**)

## Como compilar

### Dev-C++

1. Abra `server.c` e `client.c` em arquivos separados (são dois programas).
2. Vá em **Ferramentas → Opções do Compilador**, marque *"Add the following commands when calling the linker"* e adicione:
   ```
   -lws2_32
   ```
3. Compile cada um com **F11** (ou **F9**).

### Linha de comando (MinGW)

```bash
gcc server.c -o server.exe -lws2_32
gcc client.c -o client.exe -lws2_32
```

## Como jogar

1. Execute **primeiro** o `server.exe`. Ele aguarda o Herói na porta 27015.
2. Execute o `client.exe` e digite o **IP do computador que está rodando o servidor** (ou apenas ENTER para `127.0.0.1`).
3. Alternem os turnos até alguém vencer.

### Qual IP usar?

| Situação | IP |
|---|---|
| Os dois programas no mesmo PC | `127.0.0.1` |
| Mesma rede local (Wi-Fi/roteador) | IPv4 do PC do servidor (`ipconfig`) |
| Pela internet com VPN (ex.: Radmin VPN, Hamachi) | IPv4 do servidor na VPN |

## Problemas comuns

| Erro | Causa provável |
|---|---|
| `undefined reference to WSAStartup` (ou `socket`, `connect`...) | Faltou o `-lws2_32` no linker |
| Erro `10061` ao conectar | O servidor não está rodando, ou o IP/porta está errado |
| Erro `10060` ao conectar | Firewall bloqueando; permita o programa ou libere a porta TCP 27015 |
| Erro `10048` no `bind` | A porta 27015 já está em uso (feche o servidor anterior) |

## Autoria

Projeto acadêmico de Sistemas Operacionais. Adicione aqui seu nome e sua instituição.
