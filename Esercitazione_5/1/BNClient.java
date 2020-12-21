import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;

public class BNClient {
    public static void main(String[] args) {
        /** java BNClient   host    porta */
        String localita, numero_loc, ack, res;

        /** Controllo argomenti */
        if (args.length != 2) {
            System.err.println("Errore sintassi. Uso corretto: java BNClient\thost\tporta");
            System.exit(1);
        }

        try {
            /** Creazione della socket */            
            Socket sd = new Socket(args[0], Integer.parseInt(args[1]));
            BufferedReader netIn = new BufferedReader(new InputStreamReader(sd.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(sd.getOutputStream(), StandardCharsets.UTF_8));
            /** Input utente */
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));

            /** Gestione delle richieste */
            for(;;) {
                /** Richiesta delle informazioni all'utente */
                System.out.println("Inserire la località di interesse:");
                localita = kb.readLine();

                /** Controllo se località è diversa da "." */
                if (localita.equals(".")) {
                    break;
                }

                System.out.println("Inserire il numero di località di interesse:");
                numero_loc = kb.readLine();

                if (numero_loc.equals(".")) {
                    break;
                }

                /** Invio località */
                netOut.write(localita);
                netOut.newLine();
                netOut.flush();

                /** Ricevo ack */
                ack = netIn.readLine();
                if (!ack.equals("OK")) {
                    break;
                }

                /** Invio numero località */
                netOut.write(numero_loc);
                netOut.newLine();
                netOut.flush();

                /** Ricevo le informazioni dal server */
                for(;;) {
                    /** Ricevo il risultato */
                    res = netIn.readLine();
                    /** Lo stampo a video */
                    System.out.println(res);
                    /** Controllo fine ricezione */
                    if (res.equals("FINE")) {
                        break;
                    }
                }
            }

            /** Chiusura della socket */
            sd.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);
        }


    }
}