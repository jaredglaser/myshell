#include "get_path.h"

main()
{
  char cmd[64];
  struct pathelement *p;

  p = get_path();
  while (p) {         // WHERE
    sprintf(cmd, "%s/gcc", p->element);
    if (access(cmd, X_OK) == 0)
      printf("[%s]\n", cmd);
    p = p->next;
  }

  printf("----------\n");

  p = get_path();
  while (p) {         // WHICH
    sprintf(cmd, "%s/gcc", p->element);
    if (access(cmd, X_OK) == 0) {
      printf("[%s]\n", cmd);
      break;
    }
    p = p->next;
  }
}
