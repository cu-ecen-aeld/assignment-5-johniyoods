#include<syslog.h>
#include<stdio.h>

int main(int argc, char *argv[])
{
        openlog("writer logs", LOG_PID, LOG_USER);
        if(argc!=3){
                syslog(LOG_ERR, "Provide all the input parameters.. ex: writer <filename> <string>");
                goto end;
        }
        FILE *file = fopen(argv[1], "w");
        if (file == NULL) {
                syslog(LOG_ERR, "Error opening file");
                goto end;
        }
        syslog (LOG_DEBUG, "Writing %s to %s", argv[1], argv[2]);
        fprintf(file, "%s\n", argv[2]);
        fclose(file);
        closelog();
        return 0;
end:
        closelog();
        return 1;
}
