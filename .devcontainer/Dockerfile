FROM alpine:3.14

RUN apk add --update --no-cache build-base zsh curl git vim gdb
RUN mkdir /workspace
WORKDIR /workspace
RUN touch /.zshrc
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

ENTRYPOINT ["/bin/zsh"]