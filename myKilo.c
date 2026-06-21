#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


struct termios orig_termios;

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
} /* TCSAFLUSH espera input pendente e descarta oq n foi lido; re-limpa terminal quando volta pro estado original */





void enableRawMode() {

  tcgetattr(STDIN_FILENO, &orig_termios); /* Read currient attriutes into a struct */

  atexit(disableRawMode); /* Chama função pra quando retornar do main ou qnd exit(); 
                            Ta dentro de enableRaw pq é quem gerencia o estado */


  struct termios raw = orig_termios; // Copia estado atual do terminal para raw

  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | IXON );
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN );   /* Modifica strutc ~termios (pré-definida) pra desligar ECHO
                                       Off canonmode ~ readingg byte-by-byte instead of line-by-line~ */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); /* Aplica mudança do echo no terminal atual; 
                                               Limpa oq tiver no buffer~antes de read() */

}

int main () { 
  enableRawMode();
  
  while (1){
    char c = '\0';
    read(STDIN_FILENO, &c, 1); 
    if (iscntrl(c)){        /* Se for ctrl + algo, exibe apenas ASCII e não o comando 
                               (qnd o terminal lẽ o char do comando, executa ele) */
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q') break;
    }



  
  return 0;}

