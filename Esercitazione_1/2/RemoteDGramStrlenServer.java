import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class RemoteDGramStrlenServer {
    
    public static void main(String[] args) {
        
        if(args.length != 1){
            System.err.println("ERROR.\nUSAGE: java RemoteDGramStrlenServer porta");
            System.exit(1);
        }

        try {

            DatagramSocket ds = new DatagramSocket(Integer.parseInt(args[0]));
            
            while(true) {

                byte[] reqBuff = new byte[2048];
                DatagramPacket reqPacket = new DatagramPacket(reqBuff, reqBuff.length);
                // ricevo il pacchetto
                ds.receive(reqPacket);

                String rcvString = new String(reqPacket.getData(), StandardCharsets.UTF_8);
                int length = 1;
                while(rcvString.charAt(length-1) != '\n') {
                    ++length;
                }
                --length;

                String resString = "" + length;
                byte[] resBuff = resString.getBytes(StandardCharsets.UTF_8);
                DatagramPacket resPacket = new DatagramPacket(resBuff,
                                            resBuff.length, 
                                            reqPacket.getAddress(),
                                            reqPacket.getPort());

                ds.send(resPacket);
            }

        } catch(IOException e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(2);
        }
    }

}
