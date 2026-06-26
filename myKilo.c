/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f) /* Aplica bitwise-AND 00011111.
                                   Isso faz com que os 3 bits superiores do char se tornem 0.
                                   ASCII foi feita de modo que cada char se encaixe a isso */


/*** data ***/

struct editorConfig {  /** Organizar variaveis globais relacionadas ao estado do editor **/
  int screenrows;
  int screencols;
  struct termios orig_termios; // Guarda configurações originais do terminal

};

struct editorConfig E; /** Variável Global (fora de qualquer função). Nasce automaticamente com zeros **/

/*** terminal ***/

void die(const char *s) { /** Limpar a tela e exibir erro(especifico) antes de morrer **/
  write(STDOUT_FILENO, "\x1b[2J", 4); /* Age como ctrl l (não /clear!) **/
  write(STDOUT_FILENO, "\x1b[H", 3);  /* Retorna positção do cursor à coordenada 1,1 do tamanho do terminal */

  perror(s);
  exit(1);
}

void disableRawMode(){
  if  (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) /* TCSAFLUSH espera output pendente e descarta oq n foi lido do inputbuffer; */ 
      die("tcsetattr"); /* Re-limpa terminal quando volta pro estado original */

}

void enableRawMode() {

  if ( tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) (die("tcgetattr")); /* Lê atributos atuais do terminal, põe na struct, error handling */

  atexit(disableRawMode); /* Chama função pra quando retornar do main ou qnd exit(); 
                            Ta dentro de enableRaw pq é quem gerencia o estado */


  struct termios raw = E.orig_termios; // Copia estado atual do terminal para raw

  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | IXON );
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN );   /* Modifica strutc ~termios (pré-definida) pra desligar ECHO
                                                         Off canonmode ~reading byte-by-byte instead of line-by-line~ */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1; /* Terminal timout em décimos de segundo */
  
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr"); /* Aplica mudança do echo no terminal atual; 
                                                                           Limpa oq tiver no buffer~antes de read() */

}


char editorReadKey() {
  int nread;
  char c;
  while((nread= read(STDIN_FILENO, &c, 1)) != 1){ /* Pega o valor retornado por read e compara;
                                                     Mantém rodando enquanto não receber bytes;
                                                     Recebe 1 byte, loop quebra e função retorna;
                                                     Ta em *terminal* pq lida com low-level input */



    if (nread == -1 && errno != EAGAIN) die ("read"); // Se der -1 (read retornar por erro), mata programa com mensagem err
  }
  return c;
}

int getCursorPosition(int *rows, int *cols){

  if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  printf("\r\n");

  char c;
  while(read(STDOUT_FILENO, &c, 1) == 1 ){
    if(iscntrl(c)){
      printf("%d\r\n", c);
    } else{
      printf("%d ('%c')\r\n", c, c);
  }
}
  editorReadKey();

  return -1;
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;
  if (1 ||ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){ /** Função que pega tamanho do terminal 
                                                                             || ws.ws_col == 0 -> caso o terminal bug (fazendo divisão por zero)
                                                                             '1' para teste do fallback **/

    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; /** Caso ioctl dê erro, faz manual, jogando cursor para frente e para baixo até travar no fim da tela; **/
    return getCursorPosition(rows, cols);
                                                                            /** ESC [ C -> Cursor Forward
                                                                                ESC [ B -> Cursor Down 
                                                                                ESC [ H -> Não funciona porque doc não especifica o que acontece caso o cursor se mova off screen (ficando a critério do 
                                                                                emulador de terminal e descentralizando controle) **/

    
  } else {
    *cols = ws.ws_col; // Passagem por referência > ponteiros
    *rows = ws.ws_row;
    return 0;
  }
}




/*** output ***/

void editorDrawRows() {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "~\r\n", 3); /** Faz ~, pula linha e volta cursor E.screebrows vezes **/
  }
}
void editorRefreshScreen() {          /** Escape Sequences do VT100**/
  write(STDOUT_FILENO, "\x1b[2J", 4); /* Age como ctrl l (não /clear!) -limpa terminal ao entrar */
  write(STDOUT_FILENO, "\x1b[H", 3); /* Retorna posição do cursor à coordenada 1,1 do tamanho do terminal */

  editorDrawRows();

  write(STDIN_FILENO, "\x1b[H", 3); /** Retorna posição cursor à coordenada 1,1 **/

}


/*** input ***/

void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3); 
      exit(0);
      break;
  }
}

/*** init ***/

void initEditor(){  /** Inicializar todos os membros -menos o orig_termios- da struct E -e validar- **/
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize"); /** Fluxo: Função é chamada e inicializada > Retorna x > If entra em cena;
                                                                                   Ps: Ler precedência de operadores 
                                                                                   ~orig_termios é inicializado em enableRawMode**/

}


int main () { 
  enableRawMode();
  initEditor();
  
  while (1){
   editorRefreshScreen();
   editorProcessKeypress(); 
  }

  return 0;}

