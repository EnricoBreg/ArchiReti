import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;

public class VDVClient {

    public static void main(String[] args) {
        
        String line;
        String nomeVino;
        String annataVino;
        String ack;

        // Controllo argomenti
        if (args.length != 2) {
            System.err.println("Uso corretto: java VDVClient nomehost porta");
            System.exit(1);
        }

        try {

            /** Per input utente */
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));

            /** Creazione socket */ 
            Socket sd = new Socket(args[0], Integer.parseInt(args[1]));
            BufferedReader netIn = new BufferedReader(new InputStreamReader(sd.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(sd.getOutputStream(), StandardCharsets.UTF_8));
            
            for(;;) {
                /* Input utente */
                System.out.println("Inserire nome vino:");
                nomeVino = kb.readLine();
                if (nomeVino.equals(".")) {
                    break;
                }

                System.out.println("Inserire annata vino");
                annataVino = kb.readLine();
                if (annataVino.equals(".")) {
                    break;
                }

                /** Invio le informazioni al server */
                netOut.write(nomeVino);
                netOut.newLine();
                netOut.flush();

                /** ricevo ack */
                ack = netIn.readLine();
                if (!ack.equals("OK")) {
                    break;
                } 

                netOut.write(annataVino);
                netOut.newLine();
                netOut.flush();

                /** Ricevo le informazioni */
                for (;;) {
                    line = netIn.readLine();
                    /** Stampo la linea a video */
                    System.out.println(line);
                    if (line.equals("FINE")) {
                        break;
                    }
                }
            }

            /** Chiudo la socket */
            sd.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);
        }
        
    }

}