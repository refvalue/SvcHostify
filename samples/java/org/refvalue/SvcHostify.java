package org.refvalue;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;
import java.util.concurrent.atomic.AtomicBoolean;

public final class SvcHostify {
    private static final AtomicBoolean running = new AtomicBoolean(false);

    static {
        try {
            // Uses UTF-8 encoding all the way.
            System.setOut(new PrintStream(System.out, true, "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }

    /**
     * The main routine of the service.
     * 
     * @param args The input arguments.
     */
    public static void run(String[] args) {
        System.out.println("A Svchost run from Java.");
        System.out.println("All outputs to System.out will be redirected to the logging file that you configured.");
        System.out.println("Input arguments:");

        for (String item : args) {
            System.out.println(item);
        }

        final String fileName = "output_java.txt";
        final String text = "It's good to write text to your own file for logging.";

        try (FileWriter writer = new FileWriter(fileName, Charset.forName("UTF-8"))) {
            writer.write(text);
        } catch (IOException e) {
            e.printStackTrace();
        }

        running.setRelease(true);

        // The main loop of your service.
        for (int i = 0; running.getAcquire(); i++) {
            System.out.println(String.format("Hello service counter: %d", i));

            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        System.out.println("Service has stopped from Java.");
    }

    /**
     * This function will be called by the SvcHostify routine in another thread and
     * you can, for example, send a signal to your 'run' routine to stop gracefully.
     */
    public static void onStop() {
        System.out.println("A stop signal received.");
        System.out.println("Requesting a stop.");

        running.setRelease(false);
    }
}
