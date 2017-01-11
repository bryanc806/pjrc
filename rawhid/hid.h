
int rawhid_open(int max, int vid, int pid, int usage_page, int usage);
int rawhid_recv(int vid, int pid, int num, void *buf, int len, int timeout);
int rawhid_send(int vid, int pid, int num, void *buf, int len, int timeout);
void rawhid_close(int vid, int pid, int num);

