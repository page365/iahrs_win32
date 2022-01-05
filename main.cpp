#include "SerialCOM.h"
#include <windows.h>
#include <stdio.h>

// COM 포트 번호를 PC에 연결된 COM 포트 번호로 변경하십시오.
// iAHRS 센서의 기본 통신 속도는 115200 bps 입니다.
CSerialCOM com("\\\\.\\COM13", 115200);

int SendRecv(const char* command, double* returned_data, int data_length);

int main(void)
{
	com.Open();

	if (com.IsConnected()) {
		double data[10];

		for (int i = 0; i < 1000; i++) {
			int no_data = SendRecv("e\n", data, 10);	// Read Euler angle
			if (no_data >= 3) {
				printf("Euler angle = %f, %f, %f\n", data[0], data[1], data[2]);
			}

			no_data = SendRecv("q\n", data, 10);		// Read Quaternion
			if (no_data >= 4) {
				printf("Quaternion = %f, %f, %f, %f\n", data[0], data[1], data[2], data[3]);
			}
			Sleep(100);
		}
	}
}

int SendRecv(const char* command, double* returned_data, int data_length)
{
	#define COMM_RECV_TIMEOUT	10	// ms

	int command_len = strlen(command);

	com.Purge();
	com.Send(command, command_len);

	const int buff_size = 1024;
	int  recv_len = 0;
	char recv_buff[buff_size + 1];	// 마지막 EOS를 추가하기 위해 + 1

	DWORD time_start = GetTickCount();

	while (recv_len < buff_size) {
		int n = com.Recv(recv_buff + recv_len, buff_size - recv_len);
		if (n < 0) {
			// 통신 도중 오류 발생
			return -1;
		}
		else if (n == 0) {
			// 아무런 데이터도 받지 못했다. 1ms 기다려본다.
			Sleep(1);
		}
		else if (n > 0) {
			recv_len += n;

			// 수신 문자열 끝에 \r 또는 \n이 들어왔는지 체크
			if (recv_buff[recv_len - 1] == '\r' || recv_buff[recv_len - 1] == '\n') {
				break;
			}
		}

		DWORD time_current = GetTickCount();
		DWORD time_delta = time_current - time_start;

		if (time_delta >= COMM_RECV_TIMEOUT) break;
	}
	recv_buff[recv_len] = '\0';

	// 에러가 리턴 되었는지 체크한다.
	if (recv_len > 0) {
		if (recv_buff[0] == '!') {
			return -1;
		}
	}

	// 보낸 명령과 돌아온 변수명이 같은지 비교한다.
	if (strncmp(command, recv_buff, command_len - 1) == 0) {
		if (recv_buff[command_len - 1] == '=') {
			int data_count = 0;

			char* p = &recv_buff[command_len];
			char* pp = NULL;

			for (int i = 0; i < data_length; i++) {
				if (p[0] == '0' && p[1] == 'x') {	// 16 진수
					returned_data[i] = strtol(p+2, &pp, 16);
					data_count++;
				}
				else {
					returned_data[i] = strtod(p, &pp);
					data_count++;
				}

				if (*pp == ',') {
					p = pp + 1;
				}
				else {
					break;
				}
			}
			return data_count;
		}
	}
	return 0;
}
