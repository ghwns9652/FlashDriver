2월 13일 수정내역
PM_operation, PM_blockqueue 파일을 통해서 페이지 매니저를 간략하게 구현
페이지 매니저는 create에서 init, destory에서 destroy 함수를 사용하여 초기화/파괴됨
Giveme_page(0)을 통해서 쓸 물리페이지를 받아올 수 있음.

Garbage collection과의 연관
Garbage collection을 한 후 빈 블록은 Enqueue 명령어를 통해 empty_queue에 집어넣어주어야 함.
그래야 페이지 매니저가 다음 쓰기부터 비워진 블록에 접근 가능.
reserve 영역을 사용하는 경우에는 알아서 GC에서 처리하도록. 페이지 매니저는 오로지 set에서 필요한 page를 넘겨주는 데에만 사용됨. 

Block Manager와의 연관
기본적으로 Block들의 정보를 담은 Block_info 구조체의 배열을 가지고 initilize가 진행됨.
현재로써는 algorithm 내의 핵심 algorithm 파일(ex. page.c, demand.c)에서 임의로 배열 구조체를 생성해주어야 함.



