import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOError;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class CMClient {
    public static void main(String[] args) {
        /** uso: java CMClient host porta */        // autorizzazione
        String username, password, categoria, ack, autor;

        /** Controllo argomenti */
        if (args.length != 2) {
            System.err.println("Errore. Uso corretto: java CMClient host porta");
            System.exit(1);
        }

        try {
            /** Creazione socket */
            Socket sd = new Socket(args[0], Integer.parseInt(args[1]));
            BufferedReader fromServer = new BufferedReader(new InputStreamReader(sd.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter toServer = new BufferedWriter(new OutputStreamWriter(sd.getOutputStream(), StandardCharsets.UTF_8));
            /** Per input utente */    
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));

            /** Richiesta username e password all'utente */
            System.out.println("Inserire username:");
            username = kb.readLine();

            System.out.println("Inserire password:");
            password = kb.readLine();

            /** Invio informazioni al server */
            toServer.write(username);
            toServer.newLine();
            toServer.flush();

            /** Ricevo ack */
            ack = fromServer.readLine();
            if (!ack.equals("OK")) {
                System.err.println("Errore");
                System.exit(2);
            }

            toServer.write(password);
            toServer.newLine();
            toServer.flush();
            
            autor = fromServer.readLine();
            if (autor.equals("NO")) {
                System.err.println("Non autorizzato!");
                System.exit(3);
            }

            /** Ciclo di gestione delle richieste */
            for(;;) {
                /** Richiesta categoria all'utente */
                System.out.println("\nInserire la categoria ('fine' per uscire):");
                categoria = kb.readLine();
                if (categoria.equals("fine")) {
                    break;
                }

                /** Invio la categoria al server */
                toServer.write(categoria);
                toServer.newLine();
                toServer.flush();

                /** Ricezione del risultato */
                String line;
                for(;;) {
                    /** Leggo dal server */
                    line = fromServer.readLine();
                    
                    /** Controllo il valore di line */
                    if (line.equals("FINE")) {
                        break;
                    }

                    /** Stampo a video */
                    System.out.println(line);
                }
            }

            /** Chiudo la socket */
            sd.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(4);
        }
    }
}