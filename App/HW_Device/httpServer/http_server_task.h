#ifndef HTTP_SERVER_TASK_H
#define HTTP_SERVER_TASK_H

#include <stdint.h>

void http_server_SetSystemResetFlag(void);
void http_server_task(int sock);      /* 단일 소켓 처리 (내부용) */
void http_server_task_all(void);      /* S0+S6 일괄 처리 – TASK_Network 에서 호출 */

#endif
