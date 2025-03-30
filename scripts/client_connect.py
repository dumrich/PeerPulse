import socket
import tempfile
import os
import subprocess

def get_server_address():
    """Prompt user for server address and port"""
    server = input("Enter server IP address: ")
    port = input("Enter server port (default 8000): ") or "8000"
    return (server, int(port))

def main():
    # Get server address from user
    ADDR = get_server_address()
    print(f"Connecting to {ADDR[0]}:{ADDR[1]}...")

    # Create socket and connect
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client.connect(ADDR)
        print("Connected to server")

        # Receive data from server
        print("Waiting for data from server...")
        received_data = client.recv(4096)  # Adjust buffer size as needed
        
        if not received_data:
            print("No data received from server")
            return

        # Create temporary file
        with tempfile.NamedTemporaryFile(suffix='.py', delete=False) as temp_file:
            temp_file_path = temp_file.name
            temp_file.write(received_data)
            print(f"Saved received data to temporary file: {temp_file_path}")

        # Execute the temporary file in a new Python process
        print(f"Executing {temp_file_path}...")
        subprocess.run(["python", temp_file_path], check=True)
        
    except ConnectionRefusedError:
        print(f"Could not connect to {ADDR[0]}:{ADDR[1]}")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Clean up
        client.close()
        try:
            if os.path.exists(temp_file_path):
                os.unlink(temp_file_path)
        except:
            pass

if __name__ == "__main__":
    main()
