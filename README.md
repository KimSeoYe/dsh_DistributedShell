# dsh : Distributed Shell

This is a kind of multi-shell. You can use it to manage multiple terminals in a single shell.<br>
You can select the terminal by pressing Ctrl+C then choosing the id of the terminal on the dsh.<br>
If you select a terminal and sends the command on dsh, the terminal executes the command and sends the result to the dsh.<br>
If a dsh sends a particular character($) while the terminal is executing, it can terminate the execution of the command.<br>

## How to build and use

```
$ make
$ ./dsh 
$ ./terminal
```

- dsh : The server. You can manage the multipel terminals using dsh.
- terminal : clients. You can created multiple terminals.

## Description

### dsh(server)

- 명령을 입력받고 terminal에게 전송하는 역할을 하는 쓰레드와, 각 소켓에 연결되어 실행 결과를 받아오는 쓰레드들이 있다.
- 새로운 클라이언트들이 연결될 때 마다 Linked list에 임의로 생성한 ID와 해당하는 소켓의 file descriptor를 저장해둔다.
- 실행 중에 dsh에서 특정 문자($)를 보내면 terminal의 명령어 실행을 종료시킬 수 있다.
- 유저가 Ctrl+C를 누르면 원하는 terminal을 선택할 수 있다.
- Ctrl+C를 눌렀을 때 다른 terminal의 실행 결과를 출력하는 것을 멈춘다. (terminal은 계속해서 결과를 보내고 있어야 한다.)
- dsh를 종료시키면 연결되어 있는 모든 socket과의 연결을 안전하게 끊는다.


### terminal(clients)

- dsh의 명령어를 계속 listen하는 쓰레드와, 받아온 명령어를 실행하는 쓰레드가 있다.
- 실행중인 명령이 없는데 dsh로부터 명령어가 오면 해당 명령어를 실행하도록 한다.
- 실행중인 명령이 있는데 dsh로부터 다른 명령어가 왔을 경우 무시한다.
- 실행중인 명령이 있는데 dsh로부터 그 실행을 종료하라는 명령이 왔을 경우($) 명령을 실행중인 프로세스를 종료한다.
- 명령어를 실행하는 쓰레드는 새로운 프로세스를 fork()하여 excevp()를 사용해 명령어를 실행하고, parent process가 그 결과를 pipe를 통해 받아온다.
- pipe를 통해 실행 결과를 읽으며 자신의 화면에 출력하고 dsh에 전송한다.

### more…

- Global mode: 하나의 terminal이 아닌, 연결된 모든 terminal들에게 명령을 실행시킬 수 있도록 한다.
- 현재는 socket file descriptor를 해당 termina의 ID로 사용하고 있는데, 임의로 ID를 만들어줘도 좋을 것 같다.
- BUG: terminal을 바꾸기 위해 id를 두번 입력해 주어야 한다. (Multi-theading problem)
