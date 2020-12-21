import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;

// java RemoteDGramStrlen nodoServer portaServer
public class RemoteDGramStrlenClient {

    public static void main(String[] args) {
        
        if(args.length != 2) {
            System.err.println("ERROR.\nUsage: java RemoteDGramStrlen nodoServer portaServer");
            System.exit(1);
        }

        String line;
        try {
            DatagramSocket ds = new DatagramSocket();
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));
            

            System.out.println("Conta la lunghezza di una stringa\nInserire una frase ('fine' per uscire):");
            line = kb.readLine();

            while(!line.equals("fine")){
                
                line += "\n";
                // Creo il pacchetto da inviare
                byte[] reqBuff = new byte[2048];
                reqBuff = line.getBytes(StandardCharsets.UTF_8);
                DatagramPacket reqPacket = new DatagramPacket(reqBuff, /* Buffer */
                                        reqBuff.length,                /* Buffer Length */
                                        InetAddress.getByName(args[0]), /* Server address */
                                        Integer.parseInt(args[1]));     /* Port */
    
                // Invio il pacchetto
                ds.send(reqPacket);

                // Ricevo il pacchetto
                byte[] resBuff = new byte[2048];
                DatagramPacket resPacket = new DatagramPacket(resBuff, resBuff.length);
                
                ds.receive(resPacket);

                // Stampa a video del risultato
                String resString = new String(resPacket.getData(), 0, resPacket.getLength(), StandardCharsets.UTF_8);
                System.out.println("Lunghezza della stringa inserita: " + resString + " caratteri");

                // Nuova richiesta all'utente
                System.out.println("Conta la lunghezza di una stringa\nInserire una frase ('fine' per uscire):");
                line = kb.readLine();

            }
            // Chiudo la socket
            ds.close();

        } catch(IOException e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(2);
        }
    }
}