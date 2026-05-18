FROM ubuntu:latest
COPY ./a.out .
RUN chmod +x a.out
RUN ulimit -v 1048576 
CMD ["/bin/sh", "-c", "/a.out > output.txt && cat output.txt"]