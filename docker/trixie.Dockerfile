FROM debian:trixie-slim
ENV DEBIAN_FRONTEND noninteractive
COPY --chmod=644 r2cloud.gpg /etc/apt/trusted.gpg.d/r2cloud.gpg
RUN echo "deb http://r2cloud.s3.amazonaws.com trixie main" >> /etc/apt/sources.list
RUN apt-get update && apt-get install --no-install-recommends -y build-essential file valgrind cmake pkg-config libiio-dev && rm -rf /var/lib/apt/lists/*