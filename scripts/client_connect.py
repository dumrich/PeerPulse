import socket
import tempfile
import os
import subprocess
import struct
import time

PAGE_SIZE = 4096

def get_server_address():
    """Prompt user for server address and port"""
    server = "100.110.118.91"
    port = input("Enter server port (default 8000): ") or "8000"
    return (server, int(port))

def send_all(sock, data):
    """Send all data across the socket"""
    total_sent = 0
    while total_sent < len(data):
        sent = sock.send(data[total_sent:])
        if sent == 0:
            raise RuntimeError("Socket connection broken")
        total_sent += sent
    return total_sent

def main():
    # Get server address from user
    ADDR = get_server_address()
    print(f"Connecting to {ADDR[0]}:{ADDR[1]}...")

    # Create socket and connect
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 16777216)  # 16MB buffer
    try:
        client.connect(ADDR)
        print("Connected to server")

        # Receive data from server
        print("Waiting for data from server...")
        received_data = client.recv(PAGE_SIZE * 2)  # Adjust buffer size as needed
        
        if not received_data:
            print("No data received from server")
            return

        # Keep receiving until no more data or a reasonable timeout
        all_data = received_data
        client.settimeout(0.5)  # Set a short timeout for additional data
        try:
            while True:
                try:
                    chunk = client.recv(PAGE_SIZE)
                    if not chunk:
                        break
                    all_data += chunk
                    print(f"Received additional {len(chunk)} bytes...")
                except socket.timeout:
                    break
        except:
            pass
        
        client.settimeout(None)  # Reset timeout
        print(f"Total received: {len(all_data)} bytes")

        # Get the bounds values
        try:
            val2 = [int(x) for x in str(client.recv(15).decode("ascii")).split(" ")]
            print(f"Received bounds: {val2}")
        except Exception as e:
            print(f"Error parsing bounds: {e}")
            return

        # Create temp file
        with tempfile.NamedTemporaryFile(mode='w+b', suffix='.py', delete=False) as temp_file:
            temp_file.write(all_data)
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

            # Send the output back to the server
            output_data = result.stdout.encode('ascii')
            print(f"Sending {len(output_data)} bytes back to server...")
            
            # Use our send_all helper to ensure all data is sent
            bytes_sent = send_all(client, output_data)
            print(f"Successfully sent {bytes_sent} bytes")
            
            # Sleep a bit to ensure the server receives everything
            time.sleep(0.5)

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
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Close the socket properly
        try:
            # Shutdown the socket to indicate we're done sending
            client.shutdown(socket.SHUT_WR)
            time.sleep(0.5)  # Wait for server to process
            client.close()
            print("Socket closed")
        except:
            pass

if __name__ == "__main__":
    main()
