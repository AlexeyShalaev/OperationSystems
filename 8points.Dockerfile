# ����� ����� � ���������� ����� � ��� ������ ��������
FROM gcc:latest AS builder

# ������������� ������� ���������� � ����������
WORKDIR /app

# �������� �������� ����� �������� � ���������
COPY programs/8points/handler.c handler.c
COPY programs/8points/manager.c manager.c


RUN gcc -o handler handler.c
RUN gcc -o manager manager.c


CMD ./handler tests/input_1.txt tests/input_2.txt tests/output_1.txt tests/output_2.txt && ./manager
