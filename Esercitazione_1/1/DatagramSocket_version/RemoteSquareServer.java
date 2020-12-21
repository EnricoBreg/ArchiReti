import java.io.IOError;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.nio.charset.StandardCharsets;

// Usage: java RemoteSquareServer porta

public class RemoteSquareServer {
    
    public static void main(String[] args) {
        
        int reqNumber = 0;

        if(args.length != 1) {
            System.err.println("ERROR.\nCORRECT USAGE: java RemoteSquareServer Porta");
            System.exit(1);
        }

        try {

            DatagramSocket ds = new DatagramSocket(Integer.parseInt(args[0]));

            while(true){
                
                reqNumber++;

                byte[] reqBuff = new byte[2048];
                DatagramPacket reqPacket = new DatagramPacket(reqBuff, reqBuff.length);

                // Ricevo il pacchetto dal client
                ds.receive(reqPacket);
                
                System.out.println("NEW REQUEST RECEIVED from " + reqPacket.getAddress());

                String req = new String(reqPacket.getData(), 0, reqPacket.getLength(), StandardCharsets.UTF_8);

                // Converto il numero in double così da non aver problemi di calcolo
                double n = Double.parseDouble(req);
                // Calcolo del quadrato
                double square = n*n;

                // preparo il pacchetto...
                String res = "" + square;
                byte[] resBuff = res.getBytes(StandardCharsets.UTF_8);
                DatagramPacket resPacket = new DatagramPacket(resBuff, 
                                        resBuff.length, 
                                        reqPacket.getAddress(), 
                                        reqPacket.getPort());

                // ... e lo mando
                ds.send(resPacket);

                System.out.println("REQUEST " + reqNumber + " COMPLETED.");
            }

        } catch(IOException e) {
            System.out.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);
        }

    }
}
