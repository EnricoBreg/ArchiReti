import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

// USAGE: java RemoteSquareServer 51000
public class RemoteSquareServer {
    
    public static void main(String[] args) {
        
        if(args.length != 1) {
            System.err.println("ERROR.\nUSAGE: java RemoteSquareServer 51000");
            System.exit(1);
        }

        try {
            ServerSocket ss = new ServerSocket(Integer.parseInt(args[0]));

            // Creazione della socket
            Socket s = ss.accept();
            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8));

            String reqLine;
            double number, square;
            while(true) {
                
                // Leggo il numero
                reqLine = netIn.readLine();
                
                if(reqLine.equals("fine")) {
                    continue;
                }         
                number = Double.parseDouble(reqLine);

                System.out.println("Numero ricevuto " + number);

                // Calcolo il quadrato...
                square = number*number;

                System.out.println("Numero inviato: " + square);
                
                // ... ed invio il risultato
                netOut.write("" + square);
                netOut.newLine();
                netOut.flush();
            }

        } catch(IOException | NullPointerException e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(2);
        }
    }
}
