import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;


public class ClientLPDConnreuse {
    public static void main(String[] argv) {

        if (argv.length != 2) {
            System.err.println("Uso corretto: java ClientLPDConnreuse server porta");
            System.exit(1);
        }

        try{
            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            Socket s = new Socket(argv[0], Integer.parseInt(argv[1]));
            InputStream fromServer = s.getInputStream();
            OutputStream toServer  = s.getOutputStream();

            byte[] len = new byte[2];
            byte[] ack = new byte[4];

            System.out.println("Inserisci username: ");
            String username = fromUser.readLine();

            System.out.println("Inserisci password: ");
            String password = fromUser.readLine();

            byte username_utf8[] = username.getBytes(StandardCharsets.UTF_8);
            int username_len = username_utf8.length;
            len[0] = (byte)((username_len & 0xFF00) >> 8);
            len[1] = (byte)(username_len & 0x00FF);

            toServer.write(len);
            toServer.write(username_utf8);

            byte password_utf8[] = password.getBytes(StandardCharsets.UTF_8);
            int password_len = password_utf8.length;
            len[0] = (byte)((password_len & 0xFF00) >> 8);
            len[1] = (byte)(password_len & 0x00FF);

            toServer.write(len);
            toServer.write(password_utf8);

            // Verifico ACK
            fromServer.read(ack);
            if (ack[2] != 111 || ack[3] != 107) {
                System.exit(1);
            }

            do {
                System.out.println("Inserisci categoria macchine ('fine' per terminare): ");
                String categoria = fromUser.readLine();

                if (categoria.equals("fine")) {
                    break;
                }

                byte categoria_utf8[] = categoria.getBytes(StandardCharsets.UTF_8);
                int categoria_len = categoria_utf8.length;
                len[0] = (byte)((categoria_len & 0xFF00) >> 8);
                len[1] = (byte)(categoria_len & 0x00FF);

                toServer.write(len);
                toServer.write(categoria_utf8);

                fromServer.read(len);

                int toRead = (len[0] << 8) | len[1] & 0xFF;

		/* Stampo contenuto risposta a video */
		while (toRead > 0) {
                    byte[] buffer = new byte[4096];

                    int bufsize = buffer.length;
                    int sz = (toRead < bufsize) ? toRead : bufsize;

                    var nread = fromServer.read(buffer, 0, sz);
                    System.out.write(buffer, 0, nread);

                    toRead -= nread;
		}

            } while(true);

            s.close();
        }
        catch(IOException e){
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(100);
        }
    }
}
