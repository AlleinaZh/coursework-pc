import socket
import random
import time

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

words = ["calculate", "words", "everyone", "fight", "sMilE", "Miracle"]
file_names = ["Oscars", "Moments", "121212_11", "Detectives", "Hee_Hee", "PC"]

def send_request(request):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((SERVER_IP, SERVER_PORT))
        client_socket.sendall(request.encode())
        response = b""
        while True:
            chunk = client_socket.recv(4096)
            if not chunk:
                break
            response += chunk
    return response.decode()

def search_word(word):
    print(f"Searching for the word: {word}")
    request = f"GET /search?word={word} HTTP/1.1\r\nHost: {SERVER_IP}\r\nConnection: close\r\n\r\n"
    response = send_request(request)
    print(f"Server response:\n{response}\n")

def add_file(file_name, file_content):
    print(f"Adding file: {file_name} with content: {file_content}")
    body = f"file_name={file_name}&file_content={file_content}"
    request = (
        f"POST /add_file HTTP/1.1\r\n"
        f"Host: {SERVER_IP}\r\n"
        f"Content-Type: application/x-www-form-urlencoded\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"Connection: close\r\n\r\n"
        f"{body}"
    )
    response = send_request(request)
    print(f"Server response:\n{response}\n")

def main():
    word = input("Введіть слово для пошуку або натисність enter для авто вибору слова: ")
    if word == "":
        word = random.choice(words)
    search_word(word)

    file_name = input("Введіть назву файлу або натисність enter для авто вибору слова: ")
    if file_name == "":
        file_name = random.choice(file_names)
    file_content = input("Введіть контент файлу або натисність enter для авто вибору слова: ")
    if file_content == "":
        file_content = random.choice(words) + " " + random.choice(words) + " " + random.choice(words) + " " + random.choice(words)
        print(file_content)
    add_file(file_name, file_content)

    time.sleep(1)
    word = input("Введіть слово для пошуку або натисність enter для авто вибору слова: ")
    if word == "":
        word = random.choice(words)
    search_word(word)


if __name__ == "__main__":
    main()
