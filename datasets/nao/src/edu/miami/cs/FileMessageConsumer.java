package edu.miami.cs;

import java.io.*;

/**
 * Author: Sam Abeyruwan
 */
public class FileMessageConsumer implements MessageConsumer {
    PrintWriter out;


    FileMessageConsumer(File output) throws IOException {
        out = new PrintWriter(new BufferedWriter(new FileWriter(output, true)));
    }

    @Override
    public void message(String s) {
        System.out.print(s);
        out.print(s);
        out.flush();
    }
}
