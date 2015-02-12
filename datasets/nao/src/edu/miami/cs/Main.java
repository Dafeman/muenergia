package edu.miami.cs;

import gnu.io.SerialPort;

import java.io.File;
import java.io.IOException;

/**
 * Author: Sam Abeyruwan
 */
public class Main {

    public static void main(String[] args) {
        try {
            Serial serial = new Serial("/dev/ttyACM0", 115200, 'N', SerialPort.DATABITS_8, 1);
            File output = new File("fall_right.txt");
            MessageConsumer messageConsumer = new FileMessageConsumer(output);
            serial.addListener(messageConsumer);

            Thread t = new Thread() {
                public void run() {
                    //the following line will keep this app alive for 1000 seconds,
                    //waiting for events to occur and responding to them (printing incoming messages to console).
                    try {
                        Thread.sleep(1000000);
                    } catch (InterruptedException ie) {
                    }
                }
            };
            t.start();
            System.out.println("Started");

        } catch (SerialException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
