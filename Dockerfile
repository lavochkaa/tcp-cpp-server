FROM ubuntu:22.04

RUN apt-get update && apt-get install -y g++ make

WORKDIR /app
COPY src/ ./src/

RUN cd src && make

WORKDIR /app/src

EXPOSE 8080 8081
CMD ["/app/server"]
