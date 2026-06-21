#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


struct termios orig_termios;

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
} /* TCSAFLUSH espera input pendente e descarta oq n foi lido; re-limpa terminal quando volta pro estado original*/





void enableRawMode() {

  tcgetattr(STDIN_FILENO, &orig_termios); /* read currient 
                                          attriutes into a struct */
  atexit(disableRawMode); /*chama função pra quando retornar do main
  ou qnd exit(); ta dentro de enableRaw pq é quem gerencia o estado*/


  struct termios raw = orig_termios; //copia estado atual do terminal para raw

  raw.c_lflag &= ~(ECHO | ICANON);   /* modifica strutc ~termios (pré-
                               definida) pra desligar ECHO
                               off canonmode ~ readingg byte-by-byte instead of line-by-line~*/
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); /* aplica mudança do echo
                                               no terminal atual; 
                                               limpa oq tiver no buffer~antes de read()*/

}

int main () { 
  enableRawMode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  return 0;}

