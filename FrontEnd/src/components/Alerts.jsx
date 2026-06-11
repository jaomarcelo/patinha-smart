export default function Alerts({ racao }) {
  return (
    <div className="card">
      <h2>⚠️ Alertas</h2>

      {racao <= 50 ? (
        <p>Atenção: ração abaixo de 50%</p>
      ) : (
        <p>Sistema funcionando normalmente</p>
      )}
    </div>
  );
}