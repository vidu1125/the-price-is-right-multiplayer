from app.models import db

class Product(db.Model):
    __tablename__ = "products"

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(255))
    description = db.Column(db.Text)
    image = db.Column(db.String(255))
    price = db.Column(db.Integer)
    active = db.Column(db.Boolean)
    last_updated_by = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="SET NULL"))
    last_updated_at = db.Column(db.DateTime)

    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "description": self.description,
            "image": self.image,
            "price": self.price,
            "active": self.active,
        }
