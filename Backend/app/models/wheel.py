from app.models import db

class Wheel(db.Model):
    __tablename__ = "wheel"

    id = db.Column(db.Integer, primary_key=True)
    value = db.Column(db.Integer)
    last_updated_by = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="SET NULL"))
    last_updated_at = db.Column(db.DateTime)

    def to_dict(self):
        return {
            "id": self.id,
            "value": self.value,
        }
