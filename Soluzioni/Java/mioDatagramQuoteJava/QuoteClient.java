import java.io.*;
import java.net.*;
import java.nio.charset.*;
// Usage: java QuoteClient nodoServer portaServer

public class QuoteClient {
    public static void main(String[] args) {
        // Controllo degli argomenti
        if(args.length != 2) {
            System.err.println("Usage: java QuoteClient nodoServer portaServer");
            System.exit(1);
        }

        try {
            
            // Crazione della socket
            DatagramSocket ds = new DatagramSocket();

            byte[] reqBuff = new String("QUOTE").getBytes("UTF-8");
            DatagramPacket reqPacket = new DatagramPacket(reqBuff, // Buffer
                                            reqBuff.length,  // Lunghezza del buffer
                                            InetAddress.getByName(args[0]), // Nome Server
                                            Integer.parseInt(args[1])); // Porta
            
            // Mando il messaggio al server
            ds.send(reqPacket);

            byte[] resBuff = new byte[2048];
            DatagramPacket resPacket = new DatagramPacket(resBuff, resBuff.length);

            // ricevo la risposta
            ds.receive(resPacket);

            String quote = new String(resPacket.getData(), 0, resPacket.getLength(), "UTF-8");

            // Stampo la risposta a video
            System.out.println(quote);

            ds.close();

        } catch(IOException e) {
            System.out.print(e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
      }

}