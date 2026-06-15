export default function Alerts({ racao }) {
  let mensagem = "";
  let emoji = "⚠️"; // Emoji padrão

  // Garante que o valor lido é um número para a matemática não falhar
  const peso = Number(racao);

  // A lógica em cascata com os seus valores exatos:
  if (peso <= 0) {
    mensagem = "Sem comida! O pote está vazio.";
    emoji = "🚨";
  } else if (peso < 200) {
    mensagem = "Atenção: comida acabando!";
    emoji = "⚠️";
  } else if (peso <= 500) {
    mensagem = "Pote na metade.";
    emoji = "🟡";
  } else {
    // Se não caiu em nenhum dos de cima, assume que está próximo dos 1000g
    mensagem = "Pote cheio!";
    emoji = "✅";
  }

  return (
    <div className="card">
      <h2>{emoji} Alertas</h2>
      <p>{mensagem}</p>
    </div>
  );
}
