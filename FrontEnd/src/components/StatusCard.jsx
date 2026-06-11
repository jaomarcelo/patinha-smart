export default function StatusCard({
  titulo,
  valor,
  emoji
}) {
  return (
    <div className="card">
      <h2>
        {emoji} {titulo}
      </h2>

      <h3>{valor}</h3>
    </div>
  );
}