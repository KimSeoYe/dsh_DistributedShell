
// char * recv_n_data (int sock, int n) ;

void recv_n_data (int sock, char * data, int n) ;

void send_n_data (int sock, char * data, size_t len) ;

void send_int (int sock, int h) ;

int recv_int (int sock) ;

void send_string (int sock, char * data) ;

char * recv_string (int sock) ;

char * recv_n_chars (int sock, int n) ;