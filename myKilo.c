/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f) /* Aplica bitwise-AND 00011111.
                                   Isso faz com que os 3 bits superiores do char se tornem 0.
                                   ASCII foi feita de modo que cada char se encaixe a isso */


/*** data ***/

struct editorConfig {  /** Organizar variaveis globais relacionadas às configurações do editor **/
  struct termios orig_termios;

};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); /* Age como ctrl l (não /clear!) **/
  write(STDOUT_FILENO, "\x1b[H", 3);  /* Retorna positção do cursor à coordenada 1,1 do tamanho do terminal */

  perror(s);
  exit(1);
}

void disableRawMode(){
  if  (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
     /* TCSAFLUSH espera input pendente e descarta oq n foi lido; re-limpa terminal quando volta pro estado original */
      die("tcsetattr");
}

void enableRawMode() {

  if ( tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) (die("tcgetattr")); /* Read currient attriutes into a struct & error handling*/

  atexit(disableRawMode); /* Chama função pra quando retornar do main ou qnd exit(); 
                            Ta dentro de enableRaw pq é quem gerencia o estado */


  struct termios raw = E.orig_termios; // Copia estado atual do terminal para raw

  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | IXON );
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN );   /* Modifica strutc ~termios (pré-definida) pra desligar ECHO
                                                         Off canonmode ~ readingg byte-by-byte instead of line-by-line~ */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr"); /* Aplica mudança do echo no terminal atual; 
                                                                           Limpa oq tiver no buffer~antes de read() */

}


char editorReadKey() {
  int nread;
  char c;
  while((nread= read(STDIN_FILENO, &c, 1)) != -1){ /** Pega o valor retornado por read e compara;
                                                       Mantem rodando sempre que estiver recebendo bytes;
                                                       Se der -1 (read retornar por erro), mata programa com msg err
                                                       Ta em *terminal* pq lida com low-level input **/
    if (nread == -1 && errno != EAGAIN) die ("read");
  }
  return c;
}

/*** output ***/

void editorDrawRows() {
  int y;
  for (y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "~\r\n", 3); /** Faz ~, pula linha e volta cursor 24 vezes **/
  }
}
void editorRefreshScreen() {          /** Escape Sequences do VT100**/
  write(STDOUT_FILENO, "\x1b[2J", 4); /* Age como ctrl l (não /clear!) -limpa terminal ao entrar */
  write(STDOUT_FILENO, "\x1b[H", 3); /* Retorna positção do cursor à coordenada 1,1 do tamanho do terminal */

  editorDrawRows();

  write(STDIN_FILENO, "\x1b[H", 3); /** Retorna posição cursos à coordenada 1,1 **/

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

int main () { 
  enableRawMode();
  
  while (1){
   editorRefreshScreen();
   editorProcessKeypress(); 
  }

  return 0;}

