import java.io.*;
import java.net.*;
import java.nio.charset.*;

public class QuoteServer {
    
    final static String[] quotations = { 
        "Adoro i piani ben riusciti",
        "Quel tappeto dava veramente un tono all'ambiente",
        "Se ci riprovi ti stacco un braccio",
        "Questo è un colpo di genio, Leonard",
        "I fagioli comunque erano uno schifo",
        "Il mio nome è Bond, James Bond"
    };

    public static void main(String[] args) {
        
        // java QuoteServer NPorta

        // Controllo argomenti
        if(args.length != 1) {
            System.err.println("Usage: java QuoteServer Porta");
            System.exit(1);
        }

        try {
        DatagramSocket ds = new DatagramSocket(Integer.parseInt(args[0]));

        int index = 0;

        while(true) {

            byte[] reqBuff = new byte[2048];
            DatagramPacket reqPacket = new DatagramPacket(reqBuff, reqBuff.length);
            
            ds.receive(reqPacket);

            String request = new String(reqPacket.getData(), 0, reqPacket.getLength(), StandardCharsets.UTF_8);

            if(request.equals("QUOTE")) {
                
                String quote = quotations[ index % quotations.length ];

                byte[] resBuff = quote.getBytes(StandardCharsets.UTF_8);
                DatagramPacket resPacket = new DatagramPacket(resBuff, // buffer
                                            resBuff.length, // Lunghezza del buffer
                                            reqPacket.getAddress(), // Indirizzo
                                            reqPacket.getPort()); // Porta
            
                ds.send(resPacket);
            }
            index++;
        } 
        }catch(IOException e) {
            e.printStackTrace();
            System.exit(2);
        }

    }

}
