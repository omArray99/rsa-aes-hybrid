/*
 * main.cpp
 *
 * Description:
 * This program demonstrates a hybrid encryption workflow combining:
 * - AES (Advanced Encryption Standard) for efficient symmetric encryption of a plaintext message.
 * - RSA (Rivest-Shamir-Adleman) for secure asymmetric encryption of the AES key.
 *
 * Workflow:
 * 1. The user provides a plaintext file to encrypt.
 * 2. AES encrypts the plaintext, generating a random AES key and IV.
 * 3. RSA encrypts the AES key, ensuring secure transmission.
 * 4. Both ciphertexts (message and encrypted AES key) are saved to files.
 * 5. During decryption, the RSA-encrypted AES key is decrypted first.
 * 6. The decrypted AES key is used to decrypt the original message.
 *
 * Key Features:
 * - RSA implementation written from scratch (without third-party cryptography libraries).
 * - AES encryption and decryption performed using OpenSSL.
 * - PKCS#1 padding applied to RSA-encrypted data.
 * - Extensive logging for clarity and debugging.
 *
 * Note:
 * This project is for educational purposes and demonstrates RSA implementation principles.
 * It is not optimized for production use due to limitations in key size and security guarantees.
 */

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include "rsa.h"
#include "rsa_util.h"
#include "aes_util.h"
#include "file_utils.h"
#include "encoding_utils.h"

int main()
{
  rmFile(); // Clear previous log file
  Logger("Program started");

  // Get the path to the text file to encrypt
  std::string file_to_encrypt;
  std::cout << "Enter the path to your textfile: " << std::endl;
  std::cin >> file_to_encrypt;

  if (!fileExists(file_to_encrypt))
  {
    Logger("Error: File " + file_to_encrypt + " does not exist.");
    throw std::runtime_error("File not found");
  }

  // Read the file content into a string
  std::string message = getFileContent(file_to_encrypt);
  int msg_len = message.size();
  const byte *plaintext = reinterpret_cast<const byte *>(message.data());

  Logger("Plaintext file (" + file_to_encrypt + ") read successfully\n");

  Logger("===== Encryption Phase =====");

  // AES encryption setup
  Logger("Starting AES encryption of plaintext...");
  std::vector<byte> aeskey = generate16bytes(); // Generate a random AES key
  std::vector<byte> iv, aes_ciphertext, aes_decryptedtext;
  int ciphertext_len, decryptedtext_len;
  std::string aes_ciphertext_path = "ciphertext/msg_enc.aes";

  // Encrypt message with AES and savit it to a file
  ciphertext_len = aes_encrypt(plaintext, msg_len, aeskey,
                               aes_ciphertext, aes_ciphertext_path);
  Logger("AES ciphertext written to: " + aes_ciphertext_path);

  // RSA encryption for the AES key
  RSAcrypt rsa;                         // Initialize RSA cryptography object from rsa.cpp
  std::vector<uint64_t> rsa_ciphertext; // Encrypted AES key
  std::vector<byte> rsa_decryptedtext;  // Decrypted AES key
  std::string pubKey_path = "keys/pubKey.pem";
  std::string privKey_path = "keys/privKey.pem";
  std::string rsa_ciphertext_path = "ciphertext/key_enc.bin";

  // Read key pair
  uint64_t modulus, k_pub, k_priv;
  readKey(pubKey_path, modulus, k_pub);
  readKey(privKey_path, modulus, k_priv);

  Logger("RSA keys generated successfully.\n"
         "                    Public key: " +
         sha256str(std::to_string(k_pub)) +
         ",\n                    Private key: " +
         sha256str(std::to_string(k_priv)) +
         "\n                    Note: RSA keys are "
         "hashed (SHA-256) before logging for security purposes.");
  Logger("Base64 encoded public key exported to: " + pubKey_path);
  Logger("Base64 encoded private key exported to: " + privKey_path);

  // Encrypt the AES key using RSA and save it to a file
  Logger("RSA encrypting the AES key using public key...");
  rsa.rsa_encrypt(k_pub, modulus, aeskey, rsa_ciphertext, rsa_ciphertext_path);
  Logger("RSA ciphertext (encrypted AES key) saved to: " + rsa_ciphertext_path + "\n");

  Logger("===== Decryption Phase =====");

  // Decrypt the RSA-encrypted AES key
  Logger("RSA decrypting the AES key using private key...");
  rsa.rsa_decrypt(k_priv, modulus, rsa_ciphertext, rsa_decryptedtext);
  Logger("AES key successfully recovered.");

  // Read the IV and ciphertext from encrypted file
  readCiphertextIV(aes_ciphertext_path, iv, aes_ciphertext);
  Logger("AES ciphertext and Initialization Vector (IV) successfully deserialized from: " + aes_ciphertext_path);

  // Decrypt the AES-encrypted message using the decrypted AES key
  Logger("Starting AES decryption of the ciphertext...");
  decryptedtext_len = aes_decrypt(aes_ciphertext, ciphertext_len,
                                  rsa_decryptedtext, iv, aes_decryptedtext);
  Logger("AES decryption completed.");

  // Convert decrypted data back to a string and save to a file
  std::string decryptedtext_str(aes_decryptedtext.begin(), aes_decryptedtext.end());
  writeDecrytedMsg("test/decrypted.txt", decryptedtext_str);
  Logger("Decrypted message exported to: test/decrypted.txt");

  // Verify decrypted message matches original plaintext
  if (message == decryptedtext_str)
  {
    Logger("Decrypted message matches the original plaintext.\n");
  }

  Logger("Program finished successfully.");
  return 0;
}