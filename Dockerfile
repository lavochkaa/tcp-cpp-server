FROM ubuntu:22.04

RUN apt-get update && apt-get install -y g++ make

WORKDIR /app
COPY src/ ./src/

RUN cd src && make

EXPOSE 8081
CMD ["/app/server"]
