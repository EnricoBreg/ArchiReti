import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.*;
import java.nio.charset.Charset;

public class RemoteHeadClient {
    // Usage: java RemoteHeadClient hostname porta nomefile
    public static void main(String[] args) {

        // Controllo numeri parametri invocazione
        if(args.length != 3) {
            System.err.println("Usage: java RemoteHeadClient hostname porta nomefile");
            System.exit(1);
        }

        try {
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), Charset.forName("UTF-8")));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), Charset.forName("UTF-8")));

            netOut.write(args[2]);
            netOut.newLine();
            netOut.flush();

            String line;
            int line_number = 1;
            while(line_number <= 5 && (line = netIn.readLine()) != null) {
                System.out.println(line);
                line_number++;
            }

            s.close();

        } catch (NumberFormatException | IOException e) {
            System.out.println(e.getMessage());
            e.printStackTrace();
        }       
     
    }
}