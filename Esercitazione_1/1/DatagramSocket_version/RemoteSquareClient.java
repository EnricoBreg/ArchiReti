import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;

// Usage: java RemoteSquareClient hostname porta

public class RemoteSquareClient {
    
    public static void main(String[] args) {
        
        if(args.length != 2) {
            System.err.println("ERROR.\nCORRECT USAGE: java RemoteSquareClient hostname porta");
            System.exit(1);
        }

        // L'utente inserisce i numeri finchè non inserisce la stringa fine
        try {
            String req = "fine";
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));
            System.out.println("Inserisci un numero che vuoi calcolarne il quadrato (si accettano anche numeri decimali): ");
            req = kb.readLine();

            while(Pattern.matches("[a-zA-Z]+", req) && !Pattern.matches("fine", req)) {
                System.out.println("Input inserito non valido ripetere");
                req = kb.readLine();
            }
            
            while(!req.equals("fine")) {
                // Creo la socket datagram
                DatagramSocket ds = new DatagramSocket();

                byte[] reqBuf = new byte[2048];
                reqBuf = req.getBytes(StandardCharsets.UTF_8);
                
                DatagramPacket reqPacket = new DatagramPacket(reqBuf,   // buffer
                                        reqBuf.length,                  // lunghezza del buffer 
                                        InetAddress.getByName(args[0]), // Indirizzo
                                        Integer.parseInt(args[1]));     // Porta
                
                // Mando il pacchetto al server
                ds.send(reqPacket);

                // Preparo il buffer
                byte[] resBuf = new byte[2048];
                DatagramPacket resPacket = new DatagramPacket(resBuf, resBuf.length);
                
                // Ricevo il pacchetto di risposta
                ds.receive(resPacket);

                // Interpreto la risposta
                String res = new String(resPacket.getData(), 0, resPacket.getLength(), StandardCharsets.UTF_8);
                
                // Visualizzo a video la risposta
                System.out.println("Il quadrato del numero inserito è: " + res);

                // Nuova richiesta all'utente
                System.out.println("Inserisci un numero che vuoi calcolarne il quadrato (si accettano anche numeri decimali): ");
                req = kb.readLine();
                while(Pattern.matches("[a-zA-Z]+", req) && !Pattern.matches("fine", req)) {
                    System.out.println("Input inserito non valido ripetere");
                    req = kb.readLine();
                }

                ds.close();

            } // Fine while

        }  catch (IOException e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(2);
        }
    }
}
