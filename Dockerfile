ARG BUILD_FROM
FROM $BUILD_FROM

ENV LANG C.UTF-8

# Install requirements for add-on
RUN apk add --no-cache python3
RUN apk add --no-cache opencv

WORKDIR /root

# Copy data for add-on
COPY run.sh /root
COPY processNewFile.sh /root
COPY readGauge /root
RUN chmod a+x /root/run.sh
RUN chmod a+x /root/processNewFile.sh
RUN chmod a+x /root/readGauge

CMD [ "/root/run.sh" ]

