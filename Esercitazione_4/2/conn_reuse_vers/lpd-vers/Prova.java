public class Prova {
    public static void main(String[] args) {
        int string_len;
        byte[] len = new byte[2];
        String string = "HELLO GUYS";

        /** HELLO GUYS = 10 caratteri/bytes */
        string_len = string.length();

        /** Codifica della lunghezza in Network Byte Order */
        len[0] = (byte)((string_len & 0xFF00) >> 8);
        len[1] = (byte)(string_len & 0x00FF);

        /** Decodifica della lunghezza */
        int len_fin = (len[0] << 8) | len[1];

        System.out.println("La lunghezza della stringa è: " + len_fin + " caratteri");
    }
}