/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Isaac Trost - wit2102 Robert Pendergrast - rlp2153, Moises Mata - mm6155
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define HOLD_COUNT 20
#define SHIFT 2

/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

void execute_key(uint8_t, uint8_t;

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*', 23, col);
  }

  fbputs("Hello CSEE 4840 World!Test", 4, 10);
  drawHorizontalLine(10, 1);



  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  uint8_t held_char = 0;
  uint8_t held_mod = 0;
  packet prev = {0, 0, {0, 0, 0, 0, 0, 0}};
  uint8_t held_count = 0;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      uint8_t rightmost = 0;
      for(int i = 0; i < 6; i++){
        if(packet.keycode[i] == 0){
          break;
        }
        rightmost = i;
      }
      if(rightmost == 0){
        held_char = 0;
        hend_mod = 0;
        continue;
      }
      if(rightmost == held_char && packet.modifiers == held_mod){
        if(held_count < HOLD_COUNT){
          held_count++;
          continue;
        }
        execute_key(rightmost, packet.modifiers);
        continue;
      }
      uint8_t new = 1;
      for(int i = 0; i < 6; i++){
        if(packet.keycode[i] == 0){
          break;
        }
        if(prev.keycode[i] == rightmost){
          new = 0;
          break;
        }
      }
      if(new){
        execute_key(rightmost, packet.modifiers);
        held_char = rightmost;
        held_mod = packet.modifiers;
      }
      prev = packet;
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	      break;
      }
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void execute_key(uint8_t key, uint8_t modifiers, int position, String & message){
  if(key == 0x28){
    if(message.size() > 0 && modifiers == 0){
      write(sockfd, message.c_str(), message.size());
      message = "";
    } else if(modifiers == SHIFT){
    } 
  }
  if(key > 3 && key < 30){
    if(modifiers == SHIFT){
      message.append(key + 61);
    } else if (modifiers == 0){
      message.append(key + 93);
    }
  }
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    fbputs(recvBuf, 8, 0);
  }

  return NULL;
}

