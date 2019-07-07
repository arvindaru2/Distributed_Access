"""
Test the server
"""

import socket
import sys

# Format: (query, expected_response)
test_cases = [
  ("\"don't\"", "tests/logs/searchme.txt:No don't, I'm carrying!\n"),
  ("'\"'", "tests/logs/searchme.txt:This 'line' contains \"double quotes\".\n"),
  ('"\'"', ("tests/logs/searchme.txt:No don't, I'm carrying!\n"
            "tests/logs/searchme.txt:This 'line' contains \"double quotes\".\n"
            "tests/logs/searchme.txt:But this one doesn't.\n")),
  ('duplicate', ('tests/logs/searchme.txt:This line is a duplicate.\n'
                 'tests/logs/searchme.txt:This line is a duplicate.\n')),
  ('thiswillnotmatchanything', ''),
]

def print_test_case(tc):
  query, expected = tc
  print(('\tQuery: %s\n'
         '\tExpected:\t%s\n') % (query, expected)),

def sendall(sock, data):
  while data != '':
    sz = sock.send(data)
    data = data[sz:]

# From https://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data
def recvall(sock, buffer_size=4096):
  buf = sock.recv(buffer_size)
  while buf:
    yield buf
    buf = sock.recv(buffer_size)

def run_one_test(test_case):
  query, expected = test_case
  # Open a socket to the server
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect(('localhost', 5678))
  #with socket.create_connection(('127.0.0.1', 5678)) as s:
  # Send a query
  sendall(s, query + '\x00')
  # Receive the results
  response = ''.join(recvall(s))
  s.close()

  # Check the results
  # Messages end in a nul byte
  if response[-1] != chr(0) or response[:-1] != expected:
    print('Test failed:')
    print_test_case(test_case)
    print('\tActual:\t\t%s\n' % response),
    '''
    with open('actual.txt', 'w') as f:
      f.write(response)
    with open('expected.txt', 'w') as f:
      f.write(expected)
    sys.exit(1)
    '''
    return False
  else:
    print('Test passed: ' + str(test_case))
    return True

# Send tests to the server
def run_all_tests(test_cases):
  all_passed = True
  for tc in test_cases:
    if not run_one_test(tc):
      all_passed = False
  return all_passed

if __name__ == '__main__':
  if run_all_tests(test_cases):
    print
    print('All tests passed!')
  else:
    print('One or more tests failed. :(')
    sys.exit(1)

