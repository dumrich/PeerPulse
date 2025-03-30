import socket
import tempfile
import os
import subprocess
import struct

PAGE_SIZE = 4096

def get_server_address():
    """Prompt user for server address and port"""
    server = "100.110.118.91"
    port = input("Enter server port (default 8000): ") or "8000"
    return (server, int(port))

def main():
    # Get server address from user
    ADDR = get_server_address()
    print(f"Connecting to {ADDR[0]}:{ADDR[1]}...")

    # Create socket and connect
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(ADDR)
    print("Connected to server")

    # Receive data from server
    print("Waiting for data from server...")
    received_data = client.recv(PAGE_SIZE * 2)  # Adjust buffer size as needed
    
    if not received_data:
        print("No data received from server")
        return

    # Get the bounds values
    val2 = [int(x) for x in str(client.recv(15).decode("ascii")).split(" ")]
    print(f"Received bounds: {val2}")

    # Create temp file
    with tempfile.NamedTemporaryFile(mode='w+b', suffix='.py', delete=False) as temp_file:
        temp_file.write(received_data)
        temp_file_path = temp_file.name
        print(f"Created temp file: {temp_file_path}")

    # Prepare environment variables
    env = os.environ.copy()
    env.update({
        'PROCESS_BOUND_LOWER': str(val2[0]),
        'PROCESS_BOUND_UPPER': str(val2[1])
    })

    # Run the script as subprocess
    try:
        print("\nExecuting received script...")
        result = subprocess.run(
            ['python3', temp_file_path],
            env=env,
            capture_output=True,
            text=True,
            check=True
        )
        
        print("\nScript output:")
        print(result.stdout)

        client.send(result.stdout)

    except subprocess.CalledProcessError as e:
        print(f"\nScript failed with error: {e}")
        print(f"Return code: {e.returncode}")
        print(f"Output: {e.stdout}")
        print(f"Errors: {e.stderr}")
    finally:
        # Clean up temp file
        try:
            os.unlink(temp_file_path)
            print(f"\nRemoved temp file: {temp_file_path}")
        except OSError as e:
            print(f"\nError removing temp file: {e}")

if __name__ == "__main__":
    main()
