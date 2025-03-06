import socket
import time

def measure_tcp_connection(host, port, num_pings=5):
    """Measures TCP connection time to a given host and port.

    Args:
        host (str): The hostname or IP address to connect to.
        port (int): The port number to connect to.
        num_pings (int): The number of pings to send for measurement.

    Returns:
        list: A list of round-trip times (RTTs) in seconds, or None if an error occurs.
    """
    rtts = []
    for _ in range(num_pings):
        try:
            start_time = time.time()
            # Create a socket object
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Set a timeout for the connection attempt
            client_socket.settimeout(5)
            # Connect to the server
            client_socket.connect((host, port))
            end_time = time.time()
            rtt = end_time - start_time
            rtts.append(rtt)
            client_socket.close()
            time.sleep(0.1)  # Wait briefly before sending the next ping
        except socket.error as e:
            print(f"Socket error: {e}")
            return None
        except Exception as e:
             print(f"An unexpected error occurred: {e}")
             return None
    return rtts

def test_tcp_connection_time(host, port, timeout=5):
    """
    Tests the connection time to a TCP server.

    Args:
        host (str): The hostname or IP address of the server.
        port (int): The port number of the server.
        timeout (int, optional): Timeout in seconds. Defaults to 5.

    Returns:
        float: The connection time in seconds, or None if the connection failed.
    """
    start_time = time.time()
    try:
        # Create a socket object
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        
        # Attempt to connect to the server
        sock.connect((host, port))
        end_time = time.time()
        return end_time - start_time
    
    except socket.error as e:
        print(f"Connection failed: {e}")
        return None
    
    finally:
        sock.close()

if __name__ == "__main__":
    host = "www.google.com"
    port = 80
    rtts = measure_tcp_connection(host, port)

    if rtts:
        print("TCP connection times:")
        for i, rtt in enumerate(rtts):
            print(f"Ping {i+1}: {rtt:.4f} seconds")
        print(f"Average RTT: {sum(rtts)/len(rtts):.4f} seconds")

    connection_time = test_tcp_connection_time(host, port)

    if connection_time:
        print(f"Connection time to {host}:{port}: {connection_time:.4f} seconds")
    else:
        print(f"Failed to connect to {host}:{port}")
