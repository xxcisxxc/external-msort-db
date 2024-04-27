FROM gcc:12
ARG DIR=/usr/src/exmsort
COPY . ${DIR}
WORKDIR ${DIR}
RUN make -j
ENV DISTINCT=1
ENTRYPOINT ["./ExternalSort.exe"]
CMD ["-c", "1000", "-s", "500", "-o", "trace.log"]