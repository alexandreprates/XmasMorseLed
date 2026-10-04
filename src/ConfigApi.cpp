#include "ConfigApi.h"

namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
bool decode(std::string_view input, std::string& output) {
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '%') {
      if (i + 2 >= input.size() || hex(input[i + 1]) < 0 || hex(input[i + 2]) < 0) return false;
      output += static_cast<char>((hex(input[i + 1]) << 4) | hex(input[i + 2]));
      i += 2;
    } else output += input[i] == '+' ? ' ' : input[i];
  }
  return true;
}
}

ApiResponse apiError(int status, const char* code, const char* message, const char* field) {
  // All arguments are application-owned literals, never untrusted input.
  return {status, std::string("{\"error\":\"") + code + "\",\"message\":\"" + message + "\",\"field\":\"" + field + "\"}"};
}

ApiResponse configResponse(const Configuration& configuration) {
  const auto& settings = configuration.current();
  // Canonical Morse input cannot contain quotes, backslashes or control bytes.
  return {200, std::string("{\"message\":\"") + settings.message + "\",\"wpm\":" + std::to_string(settings.wpm) + "}"};
}

ApiResponse saveConfig(Configuration& configuration, std::string_view body) {
  if (body.size() > MAX_REQUEST_BODY) return apiError(413, "body_too_large", "A solicitação excede o limite de 1 KB.");
  std::string message, speed;
  bool hasMessage = false, hasSpeed = false;
  while (!body.empty()) {
    const size_t separator = body.find('&');
    const auto pair = body.substr(0, separator);
    const size_t equals = pair.find('=');
    std::string key, value;
    if (equals == std::string_view::npos || !decode(pair.substr(0, equals), key) || !decode(pair.substr(equals + 1), value)) {
      return apiError(400, "invalid_form", "Não foi possível ler os campos enviados.");
    }
    if (key == "message" && !hasMessage) { message = value; hasMessage = true; }
    else if (key == "wpm" && !hasSpeed) { speed = value; hasSpeed = true; }
    else return apiError(400, "invalid_form", "Envie apenas mensagem e velocidade, sem campos repetidos.");
    if (separator == std::string_view::npos) break;
    body.remove_prefix(separator + 1);
    if (body.empty()) return apiError(400, "invalid_form", "Formulário incompleto.");
  }
  if (!hasMessage || !hasSpeed) return apiError(400, "missing_fields", "Preencha a mensagem e a velocidade.");
  int wpm = 0;
  bool numeric = !speed.empty() && speed.size() <= 3;
  for (char c : speed) {
    if (c < '0' || c > '9') { numeric = false; break; }
    if (numeric) wpm = wpm * 10 + (c - '0');
  }
  if (!numeric) return apiError(400, "invalid_speed", "Use um número inteiro entre 5 e 40.", "wpm");
  Settings candidate;
  switch (normalizeSettings(message, wpm, candidate)) {
    case SettingsError::Speed: return apiError(400, "invalid_speed", "Use um número inteiro entre 5 e 40.", "wpm");
    case SettingsError::MessageLength: return apiError(400, "message_length", "Use até 120 caracteres.", "message");
    case SettingsError::EmptyMessage: return apiError(400, "empty_message", "Digite uma mensagem.", "message");
    case SettingsError::UnsupportedCharacter: return apiError(400, "unsupported_character", "Use letras sem acento, números e a pontuação indicada.", "message");
    case SettingsError::None: break;
  }
  const SaveResult result = configuration.save(candidate);
  if (result == SaveResult::StorageError) return apiError(500, "storage_error", "Não foi possível salvar. A configuração anterior foi mantida.");
  if (result == SaveResult::Invalid) return apiError(400, "invalid_config", "Configuração inválida.");
  auto response = configResponse(configuration);
  response.restart = true;
  return response;
}
