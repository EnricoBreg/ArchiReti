import java.io.*;
import java.net.*;
import java.nio.charset.Charset;

public class RemoteHeadServer {
    // Usage: java RemoteHeadServer porta
    public static void main(String[] args) {

        if (args.length != 1) {
            System.err.println("Usage: java RemoteHeadServer porta");
            System.exit(1);
        }

        try {
            ServerSocket ss = new ServerSocket(Integer.parseInt(args[0]));

            for(;;) {
                System.out.println("SERVER IN ATTESA DI RICHIESTA...");
                
                Socket s = ss.accept();
                
                BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), Charset.forName("UTF-8")));
                BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), Charset.forName("UTF-8")));
                
                String filename;
                filename = netIn.readLine();
                File f = new File(filename);
                if(f.exists()) {
                    String line;
                    int line_number = 1;
                    BufferedReader bfr = new BufferedReader(new FileReader(filename));

                    // Leggo il file e lo mando sulla socket
                    while(line_number <= 5 && (line = bfr.readLine()) != null) {
                        netOut.write(line);
                        netOut.newLine();
                        netOut.flush();
                        line_number++;
                    }
                }
                s.close();
            }

        } catch (NumberFormatException | IOException e) {
            e.printStackTrace();
        }
    }
}
