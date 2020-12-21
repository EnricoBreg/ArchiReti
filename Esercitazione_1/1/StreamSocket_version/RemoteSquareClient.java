import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOError;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;

// USAGE: java RemoteSquareClient hostname porta
public class RemoteSquareClient {

    public static void main(String[] args) {
        
        if(args.length != 2) {
            System.err.println("ERROR.\nUSAGE: java RemoteSquareClient hostname porta");
            System.exit(1);
        }

        try {

            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));

            // Creazione della socket
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8));

            String line;
            System.out.println("Inserire un numero: ");
            line = kb.readLine();

            while(!line.equals("fine")) {
                
                netOut.write(line);
                netOut.newLine();
                netOut.flush();

                System.out.println("Quadrato del numero inserito: ");
                System.out.println(netIn.readLine());

                System.out.println("Inserire un numero: ");
                line = kb.readLine();
            }
            
            s.close();

        } catch (IOException e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(2);
        }

    }
}