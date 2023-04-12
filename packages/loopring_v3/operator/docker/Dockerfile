FROM registry.bihutest.com/base/degate-operator-base:0.1.0

ARG VERSION

ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8

ADD docker/requirements.txt /andui/
RUN pip3 --default-timeout=5000 install -r /andui/requirements.txt

ADD operator/ /degate/operator
ADD docker/entrypoint.sh /degate/operator/
RUN chmod +x /degate/operator/*.sh
RUN mkdir -p /degate/operator/s3_data

WORKDIR /degate/operator/

ENTRYPOINT ["./entrypoint.sh"]

